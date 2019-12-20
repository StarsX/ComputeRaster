//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
Buffer<float4>		g_roVertexPos;
//Texture2D<float>	g_txBinDepthMax;

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
//RWStructuredBuffer<uint> g_rwPerBinCount;
RWStructuredBuffer<uint> g_rwTilePrimCount;
RWStructuredBuffer<TiledPrim> g_rwTiledPrimitives;

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
// Check if the tile is overlapped by a straight line with a certain thickness.
//--------------------------------------------------------------------------------------
bool IsOverlap(uint2 tile, float2 v[3])
{
	
}

//--------------------------------------------------------------------------------------
// Check if the tile is overlapped by a straight line with a certain thickness.
//--------------------------------------------------------------------------------------
bool Overlaps(uint2 tile, float4 primVPos[3])
{
	float2 v[3];
	[unroll]
	for (uint i = 0; i < 3; ++i) v[i] = primVPos[i].xy / 8.0;

	// Triangle edge equation setup.
	const float a01 = v[0].y - v[1].y;
	const float b01 = v[1].x - v[0].x;
	const float a12 = v[1].y - v[2].y;
	const float b12 = v[2].x - v[1].x;
	const float a20 = v[2].y - v[0].y;
	const float b20 = v[0].x - v[2].x;

	// Calculate barycentric coordinates at min corner.
	float3 w0, w;
	const float2 minPoint = min(v[0], min(v[1], v[2]));
	w0.x = determinant(v[1], v[2], minPoint);
	w0.y = determinant(v[2], v[0], minPoint);
	w0.z = determinant(v[0], v[1], minPoint);

	// If pixel is inside of all edges, set pixel.
	float2 dist = tile - minPoint;
	w.x = (a12 * dist.x) + (b12 * dist.y);
	w.y = (a20 * dist.x) + (b20 * dist.y);
	w.z = (a01 * dist.x) + (b01 * dist.y);
	w += w0;
	bool isOverlap = w.x >= 0.0 && w.y >= 0.0 && w.z >= 0.0;
	//isOverlap = isOverlap || (w.x <= 0.0 && w.y <= 0.0 && w.z <= 0.0);

	dist = tile + uint2(1, 0) - minPoint;
	w.x = (a12 * dist.x) + (b12 * dist.y);
	w.y = (a20 * dist.x) + (b20 * dist.y);
	w.z = (a01 * dist.x) + (b01 * dist.y);
	w += w0;
	isOverlap = isOverlap || (w.x >= 0.0 && w.y >= 0.0 && w.z >= 0.0);
	//isOverlap = isOverlap || (w.x <= 0.0 && w.y <= 0.0 && w.z <= 0.0);

	dist = tile + uint2(0, 1) - minPoint;
	w.x = (a12 * dist.x) + (b12 * dist.y);
	w.y = (a20 * dist.x) + (b20 * dist.y);
	w.z = (a01 * dist.x) + (b01 * dist.y);
	w += w0;
	isOverlap = isOverlap || (w.x >= 0.0 && w.y >= 0.0 && w.z >= 0.0);
	//isOverlap = isOverlap || (w.x <= 0.0 && w.y <= 0.0 && w.z <= 0.0);

	dist = tile + uint2(1, 1) - minPoint;
	w.x = (a12 * dist.x) + (b12 * dist.y);
	w.y = (a20 * dist.x) + (b20 * dist.y);
	w.z = (a01 * dist.x) + (b01 * dist.y);
	w += w0;
	isOverlap = isOverlap || (w.x >= 0.0 && w.y >= 0.0 && w.z >= 0.0);
	//isOverlap = isOverlap || (w.x <= 0.0 && w.y <= 0.0 && w.z <= 0.0);

	const float area = determinant(v[0], v[1], v[2]);

	return isOverlap || area < 1.0;
}

//--------------------------------------------------------------------------------------
// Appends the pointer to the first vertex together with its
// clipping parameters in clip space at the end of BfTiledCurves.
//--------------------------------------------------------------------------------------
void AppendPrimitive(uint primId, uint tileY, inout uint2 scanLine)
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
	//InterlockedAdd(g_rwPerBinCount[tiledPrim.TileIdx], 1);

	scanLine.x = 0xffffffff;
}

//--------------------------------------------------------------------------------------
// Bin the primitive.
//--------------------------------------------------------------------------------------
void BinPrimitive(float4 primVPos[3], uint primId)
{
	// Create the AABB.
	uint2 minTile, maxTile;
	ComputeAABB(primVPos, minTile, maxTile);

	const float nearestZ = min(primVPos[0].z, min(primVPos[1].z, primVPos[2].z));

	for (uint i = minTile.y; i <= maxTile.y; ++i)
	{
		uint2 scanLine = uint2(0xffffffff, 0);

		for (uint j = minTile.x; j <= maxTile.x; ++j)
		{
			const uint2 tile = uint2(j, i);
			//if (g_txBinDepthMax[tile] <= nearestZ) continue; // Depth Test failed for this tile

			// Tile overlap tests
			if (Overlaps(tile, primVPos))
				scanLine.x = scanLine.x == 0xffffffff ? j : scanLine.x;
			else scanLine.y = j;

			scanLine.y = j == maxTile.x ? j : scanLine.y;
				
			if (scanLine.x < scanLine.y)
				AppendPrimitive(primId, i, scanLine);
		}
	}
}

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
