//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SharedConst.h"
#include "Common.hlsli"

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
StructuredBuffer<TilePrim> g_roBinPrimitives;

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<float4> g_rwVertexPos;
RWStructuredBuffer<uint> g_rwTilePrimCount;
RWStructuredBuffer<TilePrim> g_rwTilePrimitives;

globallycoherent
RWTexture2D<uint> g_rwTileZ;
RWTexture2D<uint> g_rwHiZ;

[numthreads(8, 8, 1)]
void main(uint2 GTid : SV_GroupThreadID, uint Gid : SV_GroupID)//, uint GTidx : SV_GroupIndex)
{
	TilePrim tilePrim = g_roBinPrimitives[Gid];
	const uint2 bin = uint2(tilePrim.TileIdx % g_binDim.x, tilePrim.TileIdx / g_binDim.x);

	float3x4 primVPos;

	// Load the vertex positions of the triangle
	const uint baseVIdx = tilePrim.PrimId * 3;
	[unroll]
	for (uint i = 0; i < 3; ++i) primVPos[i] = g_rwVertexPos[baseVIdx + i];

	// To screen space.
	ToScreenSpace(primVPos);

#if RE_HI_Z
	const uint zMin = asuint(min(primVPos[0].z, min(primVPos[1].z, primVPos[2].z)));
	if (g_rwHiZ[bin] < zMin) return;
#endif

	// Scale the primitive for conservative rasterization.
	float3x2 v;
	[unroll]
	for (i = 0; i < 3; ++i) v[i] = primVPos[i].xy / TILE_SIZE;
	float3x2 sv = Scale(v, 0.5);

	float3 w;
	const uint2 tile = (bin << TILE_TO_BIN_LOG) + GTid;
	const float2 pos = tile + 0.5;
	if (!Overlap(pos, sv, w)) return;

#if HI_Z
	// Depth test
#if !RE_HI_Z
	const uint zMin = asuint(min(primVPos[0].z, min(primVPos[1].z, primVPos[2].z)));
#endif
	const uint zMax = asuint(max(primVPos[0].z, max(primVPos[1].z, primVPos[2].z)));

	// Shrink the primitive.
	const float area = determinant(v[0], v[1], v[2]);
	sv = Scale(v, -0.5);
	
	uint tileZ;
	if (area >= 2.0 && Overlap(pos, sv, w))
		InterlockedMin(g_rwTileZ[tile], zMax, tileZ);
	else
	{
		DeviceMemoryBarrier();
		tileZ = g_rwTileZ[tile];
	}

	if (tileZ < zMin) return;
#endif

	tilePrim.TileIdx = g_tileDim.x * tile.y + tile.x;

	uint idx;
#if SHADER_MODEL >= 6
	// compute number of items to append for the whole wave
	const uint appendCount = WaveActiveCountBits(true);
	// update the output location for this whole wave
	if (WaveIsFirstLane())
	{
		// this way, we only issue one atomic for the entire wave, which reduces contention
		// and keeps the output data for each lane in this wave together in the output buffer
		InterlockedAdd(g_rwTilePrimCount[0], appendCount, idx);
	}
	idx = WaveReadLaneFirst(idx);		// broadcast value
	idx += WavePrefixCountBits(true);	// and add in the offset for this lane
	// write to the offset location for this lane
#else
	InterlockedAdd(g_rwTilePrimCount[0], 1, idx);
#endif
	g_rwTilePrimitives[idx] = tilePrim;
}
