//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

static uint g_outTileDim = g_tileDim.y;

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
Buffer<float4> g_roVertexPos;
StructuredBuffer<TilePrim> g_roBinPrimitives;

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<uint> g_rwTilePrimCount;
RWStructuredBuffer<TilePrim> g_rwTilePrimitives;

globallycoherent
RWTexture2D<uint> g_rwHiZ;

[numthreads(8, 8, 1)]
void main(uint2 GTid : SV_GroupThreadID, uint Gid : SV_GroupID)//, uint GTidx : SV_GroupIndex)
{
	TilePrim tilePrim = g_roBinPrimitives[Gid];
	const uint2 bin = uint2(tilePrim.TileIdx % g_tileDim.x, tilePrim.TileIdx / g_tileDim.x);

	float3x4 primVPos;

	// Load the vertex positions of the triangle
	const uint baseVIdx = tilePrim.PrimId * 3;
	[unroll]
	for (uint i = 0; i < 3; ++i) primVPos[i] = g_roVertexPos[baseVIdx + i];

	// To screen space.
	ToScreenSpace(primVPos);

	// Scale the primitive for conservative rasterization.
	float3x2 v;
	[unroll]
	for (i = 0; i < 3; ++i) v[i] = primVPos[i].xy / 8.0;
	float3x2 sv = Scale(v, 0.5);

	float3 w;
	const uint2 tile = (bin << 3) + GTid;
	const float2 pos = tile + 0.5;
	if (!Overlap(pos, sv, w)) return;

#if HI_Z
	// Depth test
	const uint zMin = asuint(min(primVPos[0].z, min(primVPos[1].z, primVPos[2].z)));
	const uint zMax = asuint(max(primVPos[0].z, max(primVPos[1].z, primVPos[2].z)));

	// Shrink the primitive.
	const float area = determinant(v[0], v[1], v[2]);
	sv = Scale(v, -0.5);
	
	uint hiZ;
	if (area >= 2.0 && Overlap(pos, sv, w))
		InterlockedMin(g_rwHiZ[tile], zMax, hiZ);
	else
	{
		DeviceMemoryBarrier();
		hiZ = g_rwHiZ[tile];
	}

	if (hiZ < zMin) return;
#endif

	tilePrim.TileIdx = g_outTileDim * tile.y + tile.x;

	uint idx;
	InterlockedAdd(g_rwTilePrimCount[0], 1, idx);
	g_rwTilePrimitives[idx] = tilePrim;
}
