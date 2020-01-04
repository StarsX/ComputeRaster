//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SharedConst.h"
#include "Common.hlsli"

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct TileInfo
{
	uint SizeLog;
	uint Size;
	uint2 Dim;
};

struct RasterUavInfo
{
	RWStructuredBuffer<uint> rwPrimCount;
	RWStructuredBuffer<TilePrim> rwPrimitives;
	RWTexture2D<uint> rwHiZ;
};

struct RasterInfo
{
	float3 w;
	float3x2 n;
	float2 MinPt;
	uint2 MinTile;
	uint2 MaxTile;
	uint ZMin;
	uint ZMax;
	RasterUavInfo UavInfo;
};

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<float4> g_rwVertexPos;
RWStructuredBuffer<uint> g_rwTilePrimCount;
RWStructuredBuffer<TilePrim> g_rwTilePrimitives;

globallycoherent
RWTexture2D<uint> g_rwTileZ;
globallycoherent
RWTexture2D<uint> g_rwBinZ;

RWStructuredBuffer<uint> g_rwBinPrimCount;
RWStructuredBuffer<TilePrim> g_rwBinPrimitives;

//--------------------------------------------------------------------------------------
// Cull a primitive to the view frustum defined in clip space.
//--------------------------------------------------------------------------------------
bool CullPrimitive(float3x4 primVPos)
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
void ComputeAABB(float3x4 primVPos, out uint2 minTile,
	out uint2 maxTile, TileInfo tileInfo)
{
	const float2 minPt = min(primVPos[0].xy, min(primVPos[1].xy, primVPos[2].xy));
	const float2 maxPt = max(primVPos[0].xy, max(primVPos[1].xy, primVPos[2].xy));

	minTile = floor(minPt);
	maxTile = floor(maxPt - 0.5);

	// Shrink by (tileInfo.Size x tileInfo.Size)
	minTile >>= tileInfo.SizeLog;
	maxTile >>= tileInfo.SizeLog;

	minTile = max(minTile, 0);
	maxTile = min(maxTile + 1, tileInfo.Dim);
}

//--------------------------------------------------------------------------------------
// Appends the pointer to the first vertex together with its
// clipping parameters in clip space at the end of uavInfo.rwPrimitives.
//--------------------------------------------------------------------------------------
void AppendPrimitive(uint primId, uint tileY, inout uint3 scanLine,
	uint tileDimX, RasterUavInfo uavInfo)
{
	const uint scanLineLen = scanLine.y - scanLine.x;

	TilePrim tilePrim;
	tilePrim.TileIdx = tileDimX * tileY + scanLine.x;
	tilePrim.PrimId = primId;

	uint baseIdx;
#if SHADER_MODEL >= 6
	// compute number of items to append for the whole wave
	const uint appendCount = WaveActiveSum(scanLineLen);
	// update the output location for this whole wave
	if (WaveIsFirstLane())
	{
		// this way, we only issue one atomic for the entire wave, which reduces contention
		// and keeps the output data for each lane in this wave together in the output buffer
		InterlockedAdd(uavInfo.rwPrimCount[0], appendCount, baseIdx);
	}
	baseIdx = WaveReadLaneFirst(baseIdx);	// broadcast value
	baseIdx += WavePrefixSum(scanLineLen);	// and add in the offset for this lane
	// write to the offset location for this lane
#else
	InterlockedAdd(uavInfo.rwPrimCount[0], scanLineLen, baseIdx);
#endif
	for (uint i = 0; i < scanLineLen; ++i)
	{
		uavInfo.rwPrimitives[baseIdx + i] = tilePrim;
		++tilePrim.TileIdx;
	}

	scanLine.x = scanLine.y;
	scanLine.y = scanLine.z;
}

//--------------------------------------------------------------------------------------
// Bin the primitive.
//--------------------------------------------------------------------------------------
void BinPrimitive(uint primId, uint tileDimX, RasterInfo rasterInfo)
{
	const float3 w = rasterInfo.w;
	const float3x2 n = rasterInfo.n;
	const float2 minPt = rasterInfo.MinPt;
	const uint2 minTile = rasterInfo.MinTile;
	const uint2 maxTile = rasterInfo.MaxTile;
	const uint zMin = rasterInfo.ZMin;
	const uint zMax = rasterInfo.ZMax;
	const RasterUavInfo uavInfo = rasterInfo.UavInfo;

	uint2 tile;
	for (uint i = minTile.y; i <= maxTile.y; ++i)
	{
		uint3 scanLine = uint3(0xffffffff, 0u.xx);
		tile.y = i;

		for (uint j = minTile.x; j <= maxTile.x; ++j)
		{
			// Tile overlap tests
			tile.x = j;
			if (Overlap(tile + 0.5, n, minPt, w))
				scanLine.x = scanLine.x == 0xffffffff ? j : scanLine.x;
			else scanLine.y = j;

			scanLine.y = j == maxTile.x ? j : scanLine.y;
			if (scanLine.x < scanLine.y) break;
		}

		scanLine.z = scanLine.y;
		const uint loopCount = scanLine.z - scanLine.x;
		const bool isInsideY = i + 2 > minTile.y && i + 2 < maxTile.y;

		[allow_uav_condition]
		for (uint k = 0; k < loopCount && scanLine.x < scanLine.y; ++k)	// Avoid time-out
		//while (scanLine.x < scanLine.y)
		{
#if HI_Z
			uint hiZ;
			[allow_uav_condition]
			for (uint j = scanLine.x; j < scanLine.y; ++j)
			{
				tile.x = j;
				if (j > scanLine.x + 2 && j + 3 < scanLine.y && isInsideY)
					InterlockedMin(uavInfo.rwHiZ[tile], zMax, hiZ);
				else
				{
					DeviceMemoryBarrier();
					hiZ = uavInfo.rwHiZ[tile];
				}

				if (hiZ < zMin)
				{
					// Depth Test failed for this tile
					scanLine.y = j;
					break;
				}
			}
#endif
			AppendPrimitive(primId, i, scanLine, tileDimX, uavInfo);
		}
	}
}

//--------------------------------------------------------------------------------------
// Get tile info.
//--------------------------------------------------------------------------------------
bool GetTileInfo(float3x4 primVPos, out TileInfo tileInfo)
{
	const float area = determinant(primVPos[0].xy, primVPos[1].xy, primVPos[2].xy);
	if (USE_TRIPPLE_RASTER && area > (TILE_SIZE * TILE_SIZE)* (4.0 * 4.0))
	{
		// If the area > 4x4 tile sizes, the bin rasterization will be triggered.
		tileInfo.SizeLog = BIN_SIZE_LOG;
		tileInfo.Size = BIN_SIZE;
		tileInfo.Dim = g_binDim;

		return true;
	}
	else
	{
		// Otherwise, the tile rasterization is directly done.
		tileInfo.SizeLog = TILE_SIZE_LOG;
		tileInfo.Size = TILE_SIZE;
		tileInfo.Dim = g_tileDim;

		return false;
	}
}

//--------------------------------------------------------------------------------------
// Determine all potentially overlapping tiles.
//--------------------------------------------------------------------------------------
void ProcessPrimitive(float3x4 primVPos, uint primId)
{
	// Get tile info
	TileInfo tileInfo;
	const bool useBin = GetTileInfo(primVPos, tileInfo);

	RasterInfo rasterInfo;

	// Create the AABB.
	ComputeAABB(primVPos, rasterInfo.MinTile, rasterInfo.MaxTile, tileInfo);

	rasterInfo.ZMin = asuint(min(primVPos[0].z, min(primVPos[1].z, primVPos[2].z)));
	rasterInfo.ZMax = asuint(max(primVPos[0].z, max(primVPos[1].z, primVPos[2].z)));

	// Scale the primitive for conservative rasterization.
	float3x2 v;
	[unroll]
	for (uint i = 0; i < 3; ++i) v[i] = primVPos[i].xy / tileInfo.Size;
	v = Scale(v, 0.5);

	// Triangle edge equation setup.
	rasterInfo.n = float3x2
	(
		v[1].y - v[2].y, v[2].x - v[1].x,
		v[2].y - v[0].y, v[0].x - v[2].x,
		v[0].y - v[1].y, v[1].x - v[0].x
	);

	// Calculate barycentric coordinates at min corner.
	const float2 minPt = min(v[0], min(v[1], v[2]));
	rasterInfo.w.x = determinant(v[1], v[2], minPt);
	rasterInfo.w.y = determinant(v[2], v[0], minPt);
	rasterInfo.w.z = determinant(v[0], v[1], minPt);
	rasterInfo.MinPt = minPt;

	if (useBin)
	{
		// Need bin rasterization.
		rasterInfo.UavInfo.rwPrimCount = g_rwBinPrimCount;
		rasterInfo.UavInfo.rwPrimitives = g_rwBinPrimitives;
		rasterInfo.UavInfo.rwHiZ = g_rwBinZ;
		BinPrimitive(primId, tileInfo.Dim.x, rasterInfo);
	}
	else
	{
		// Otherwise, the tile rasterization is directly done.
		rasterInfo.UavInfo.rwPrimCount = g_rwTilePrimCount;
		rasterInfo.UavInfo.rwPrimitives = g_rwTilePrimitives;
		rasterInfo.UavInfo.rwHiZ = g_rwTileZ;
		BinPrimitive(primId, tileInfo.Dim.x, rasterInfo);
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
void ToTiledSpace(float3x4 primVPos, out float2 v[3], out float2 rangeY)
{
	float2 p[3];
	[unroll]
	for (uint i = 0; i < 3; ++i) p[i] = primVPos[i].xy / TILE_SIZE;

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
void BinPrimitive2(float3x4 primVPos, uint primId)
{
	// Get tile info
	TileInfo tileInfo;
	const bool useBin = GetTileInfo(primVPos, tileInfo);

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

	RasterUavInfo uavInfo;
	//if (useBin)
	// else
	{
		// Otherwise, the tile rasterization is directly done.
		uavInfo.rwPrimCount = g_rwTilePrimCount;
		uavInfo.rwPrimitives = g_rwTilePrimitives;
		uavInfo.rwHiZ = g_rwTileZ;
	}

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

		AppendPrimitive(primId, y, scanLine, g_tileDim.x, uavInfo);

		//scan.x += phase2.x ? e12.x : e01.x;
		//scan.y += phase2.y ? e21.x : e02.x;
	}
}
#endif

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	float3x4 primVPos;

	// Load the vertex positions of the triangle
	const uint baseVIdx = DTid * 3;
	[unroll]
	for (uint i = 0; i < 3; ++i) primVPos[i] = g_rwVertexPos[baseVIdx + i];

	// Cull the primitive.
	if (CullPrimitive(primVPos)) return;

	// To screen space.
	ToScreenSpace(primVPos);

	// Store each successful clipping result.
	ProcessPrimitive(primVPos, DTid);
}
