//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

#define HI_Z 1

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
Buffer<float4>		g_roVertexPos;

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<uint> g_rwTilePrimCount;
RWStructuredBuffer<TiledPrim> g_rwTiledPrimitives;

globallycoherent
RWTexture2D<uint> g_rwHiZ;

//--------------------------------------------------------------------------------------
// Clips a primitive to the view frustum defined in clip space.
//--------------------------------------------------------------------------------------
bool ClipPrimitive(float4 primVPos[3])
{
	bool isFullOutside = true;

	[unroll]
	for (uint i = 0; i < 3; ++i)
	{
		bool isOutside = false;
		isOutside = isOutside || abs(primVPos[i].x) > primVPos[i].w;
		isOutside = isOutside || abs(primVPos[i].y) > primVPos[i].w;
		isOutside = isOutside || primVPos[i].z < 0.0;
		isOutside = isOutside || primVPos[i].z > primVPos[i].w;
		isFullOutside = isFullOutside && isOutside;
	}

	return isFullOutside;
}

//--------------------------------------------------------------------------------------
// Compute the minimum pixel as well as the maximum pixel
// possibly overlapped by the primitive.
//--------------------------------------------------------------------------------------
void ComputeAABB(float4 primVPos[3], out uint2 minTile, out uint2 maxTile)
{
	const float2 minPt = min(primVPos[0].xy, min(primVPos[1].xy, primVPos[2].xy));
	const float2 maxPt = max(primVPos[0].xy, max(primVPos[1].xy, primVPos[2].xy));

	minTile = floor(minPt);
	maxTile = floor(maxPt - 0.5);

	// Shrink by 8x8
	minTile >>= 3;
	maxTile >>= 3;

	minTile = max(minTile, 0);
	maxTile = min(maxTile + 1, g_tileDim - 1);
}

//--------------------------------------------------------------------------------------
// Move the vertex for conservative rasterization.
//--------------------------------------------------------------------------------------
float2 Conserve(float2 pv, float2 cv, float2 nv)
{
	float3 plane0 = cross(float3(cv - pv, 0.0), float3(pv, 1.0));
	float3 plane1 = cross(float3(nv - cv, 0.0), float3(cv, 1.0));

	plane0.z -= dot(0.5, abs(plane0.xy));
	plane1.z -= dot(0.5, abs(plane1.xy));

	const float3 result = cross(plane0, plane1);

	return result.xy / result.z;
}

//--------------------------------------------------------------------------------------
// Move the vertices for conservative rasterization.
//--------------------------------------------------------------------------------------
void Conserves(out float2 cv[3], float4 primVPos[3])
{
	float2 v[3];
	[unroll]
	for (uint i = 0; i < 3; ++i) v[i] = primVPos[i].xy / 8.0;
	cv[0] = Conserve(v[2], v[0], v[1]);
	cv[1] = Conserve(v[0], v[1], v[2]);
	cv[2] = Conserve(v[1], v[2], v[0]);
}

//--------------------------------------------------------------------------------------
// Check if the tile is overlapped by a straight line with a certain thickness.
//--------------------------------------------------------------------------------------
bool Overlap(float2 tile, float3x2 n, float2 minPoint, float3 w)
{
	// If pixel is inside of all edges, set pixel.
	const float2 disp = tile + 0.5 - minPoint;
	w += mul(n, disp);

	return  w.x >= 0.0 && w.y >= 0.0 && w.z >= 0.0;
}

//--------------------------------------------------------------------------------------
// Appends the pointer to the first vertex together with its
// clipping parameters in clip space at the end of BfTiledCurves.
//--------------------------------------------------------------------------------------
void AppendPrimitive(uint primId, uint tileY, inout uint3 scanLine)
{
	const uint scanLineLen = scanLine.y - scanLine.x;

	TiledPrim tiledPrim;
	tiledPrim.TileIdx = g_tileDim.x * tileY + scanLine.x;
	tiledPrim.PrimId = primId;

	uint baseIdx;
	InterlockedAdd(g_rwTilePrimCount[0], scanLineLen, baseIdx);
	for (uint i = 0; i < scanLineLen; ++i)
	{
		g_rwTiledPrimitives[baseIdx + i] = tiledPrim;
		++tiledPrim.TileIdx;
	}

	scanLine.x = scanLine.y;
	scanLine.y = scanLine.z;
}

//--------------------------------------------------------------------------------------
// Bin the primitive.
//--------------------------------------------------------------------------------------
void BinPrimitive(float4 primVPos[3], uint primId)
{
	// Create the AABB.
	uint2 minTile, maxTile;
	ComputeAABB(primVPos, minTile, maxTile);

	const uint zMin = asuint(min(primVPos[0].z, min(primVPos[1].z, primVPos[2].z)));
	const uint zMax = asuint(max(primVPos[0].z, max(primVPos[1].z, primVPos[2].z)));

	// Move the vertices for conservative rasterization.
	float2 v[3];
	Conserves(v, primVPos);

	// Triangle edge equation setup.
	const float3x2 n =
	{
		v[1].y - v[2].y, v[2].x - v[1].x,
		v[2].y - v[0].y, v[0].x - v[2].x,
		v[0].y - v[1].y, v[1].x - v[0].x,
	};
	
	// Calculate barycentric coordinates at min corner.
	float3 w;
	const float2 minPoint = min(v[0], min(v[1], v[2]));
	w.x = determinant(v[1], v[2], minPoint);
	w.y = determinant(v[2], v[0], minPoint);
	w.z = determinant(v[0], v[1], minPoint);

	uint2 tile;
	for (uint i = minTile.y; i <= maxTile.y; ++i)
	{
		uint3 scanLine = uint3(0xffffffff, 0u.xx);
		tile.y = i;

		for (uint j = minTile.x; j <= maxTile.x; ++j)
		{
			// Tile overlap tests
			tile.x = j;
			if (Overlap(tile, n, minPoint, w))
				scanLine.x = scanLine.x == 0xffffffff ? j : scanLine.x;
			else scanLine.y = j;

			scanLine.y = j == maxTile.x ? j : scanLine.y;
			if (scanLine.x < scanLine.y) break;
		}

		scanLine.z = scanLine.y;
		const uint loopCount = scanLine.z - scanLine.x;

		[allow_uav_condition]
		for (uint k = 0; k < loopCount && scanLine.x < scanLine.y; ++k)	// Avoid time-out
		//while (scanLine.x < scanLine.y)
		{
#if HI_Z
			[allow_uav_condition]
			for (uint j = scanLine.x; j < scanLine.y; ++j)
			{
				uint hiZ;
				tile.x = j;
				if (j > scanLine.x + 1 && j + 2 < scanLine.y && i > minTile.y&& i < maxTile.y)
					InterlockedMin(g_rwHiZ[tile], zMax, hiZ);
				else
				{
					DeviceMemoryBarrier();
					hiZ = g_rwHiZ[tile];
				}

				if (hiZ < zMin)
				{
					// Depth Test failed for this tile
					scanLine.y = j;
					break;
				}
			}
#endif
			AppendPrimitive(primId, i, scanLine);
		}
	}
}

#if 0
//--------------------------------------------------------------------------------------
// Select min X
//--------------------------------------------------------------------------------------
uint GetMinX(uint i, uint j, float2 v[3])
{
	return v[i].x < v[j].x ? i : j;
}

//--------------------------------------------------------------------------------------
// Select top left as the first
//--------------------------------------------------------------------------------------
uint GetV0(uint i, uint j, float2 v[3])
{
	return v[i].y < v[j].y ? i : (v[i].y == v[j].y ? GetMinX(i, j, v) : j);
}

//--------------------------------------------------------------------------------------
// Sort vertices and compute the minimum pixel as well as
// the maximum pixel possibly overlapped by the primitive.
//--------------------------------------------------------------------------------------
void ToTiledSpace(float4 primVPos[3], out float2 v[3], out float2 rangeY)
{
	float2 p[3];
	[unroll]
	for (uint i = 0; i < 3; ++i) p[i] = primVPos[i].xy / 8.0;

	rangeY.x = min(p[0].y, min(p[1].y, p[2].y));
	rangeY.y = max(p[0].y, max(p[1].y, p[2].y));
	rangeY.x = max(floor(rangeY.x) + 0.5, 0.0);
	rangeY.y = min(ceil(rangeY.y) + 0.5, g_tileDim.y);

	const uint k = GetV0(GetV0(0, 1, p), 2, p);
	v[0] = p[k];
	v[1] = p[k + 2 < 3 ? k + 2 : k + 2 - 3];
	v[2] = p[k + 1 < 3 ? k + 1 : k + 1 - 3];
}

//--------------------------------------------------------------------------------------
// Compute k and b for x = ky + b.
//--------------------------------------------------------------------------------------
float2 ComputeKB(float2 v0, float2 v1)
{
	const float2 e = v1 - v0;
	const float k = e.x / e.y;

	return float2(k, v0.x - k * v0.y);
}

//--------------------------------------------------------------------------------------
// Bin the primitive.
//--------------------------------------------------------------------------------------
void BinPrimitive2(float4 primVPos[3], uint primId)
{
	// To tiled space.
	float2 v[3], rangeY;
	ToTiledSpace(primVPos, v, rangeY);

	const float nearestZ = min(primVPos[0].z, min(primVPos[1].z, primVPos[2].z));

	const float2 e01 = ComputeKB(v[0], v[1]);
	const float2 e02 = ComputeKB(v[0], v[2]);
	const float2 e12 = ComputeKB(v[1], v[2]);
	const float2 e21 = float2(-e12.x, v[2].x + e12.x * v[2].y);

	float2 scan;
	//scan.x = e01.x * rangeY.x + e01.y;
	//scan.y = e02.x * rangeY.y + e02.y;

	bool2 phase2 = false;
	for (float y = rangeY.x; y <= rangeY.y; ++y)
	{
		if (!phase2.x && y > v[1].y)
		{
			//scan.x = e12.x * y + e12.y;
			phase2.x = true;
		}

		if (!phase2.y && y > v[2].y)
		{
			//scan.y = e21.x * y + e21.y;
			phase2.y = true;
		}

		scan.x = phase2.x ? e12.x * y + e12.y : e01.x * y + e01.y;
		scan.y = phase2.y ? e21.x * y + e21.y : e02.x * y + e02.y;

		uint3 scanLine;
		scanLine.x = min(scan.x, scan.y) - 0.5;
		scanLine.y = max(scan.x, scan.y) + 1.5;

		// Clip
		scanLine.x = max(scanLine.x, 0);
		scanLine.y = min(scanLine.y, g_tileDim.x);

		scanLine.z = scanLine.y;
		AppendPrimitive(primId, y, scanLine);

		//scan.x += phase2.x ? e12.x : e01.x;
		//scan.y += phase2.y ? e21.x : e02.x;
	}
}
#endif

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	float4 primVPos[3];

	// Load the vertex positions of the triangle
	const uint baseVIdx = DTid * 3;
	[unroll]
	for (uint i = 0; i < 3; ++i) primVPos[i] = g_roVertexPos[baseVIdx + i];

	// Clip the primitive.
	if (ClipPrimitive(primVPos)) return;

	// Store each successful clipping result.
	ToScreenSpace(primVPos);
	BinPrimitive(primVPos, DTid);
}
