//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SharedConst.h"
#define main PSMain
#include "PixelShader.hlsl"
#undef main
#include "Common.hlsli"

#define CR_PRIMITIVE_VERTEX_ATTRIBUTE_TYPE(t, c) t##3x##c
#define CR_ATTRIBUTE_GEN_TYPE(t, c) t##c
#define CR_ATTRIBUTE_TYPE(n) CR_ATTRIBUTE_GEN_TYPE(CR_ATTRIBUTE_BASE_TYPE##n, CR_ATTRIBUTE_COMPONENT_COUNT##n)

#define COMPUTE_ATTRIBUTE_float(n) \
	{ \
		CR_PRIMITIVE_VERTEX_ATTRIBUTE_TYPE(CR_ATTRIBUTE_BASE_TYPE##n, CR_ATTRIBUTE_COMPONENT_COUNT##n) primVAtt; \
		[unroll] \
		for (i = 0; i < 3; ++i) primVAtt[i] = g_roVertexAtt##n[baseVIdx + i]; \
		input.CR_ATTRIBUTE##n = mul(persp, primVAtt); \
	}

#define COMPUTE_ATTRIBUTE_min16float(n) COMPUTE_ATTRIBUTE_float(n)
#define COMPUTE_ATTRIBUTE_int(n) input.CR_ATTRIBUTE##n = g_roVertexAtt##n[baseVIdx]
#define COMPUTE_ATTRIBUTE_uint(n) COMPUTE_ATTRIBUTE_int(n)
#define COMPUTE_ATTRIBUTE_min16int(n) COMPUTE_ATTRIBUTE_int(n)
#define COMPUTE_ATTRIBUTE_min16uint(n) COMPUTE_ATTRIBUTE_int(n)
#define COMPUTE_ATTRIBUTE_bool(n) COMPUTE_ATTRIBUTE_int(n)

#define COMPUTE_ATTRIBUTE(t, n) COMPUTE_ATTRIBUTE_##t(n)
#define SET_ATTRIBUTE(n) COMPUTE_ATTRIBUTE(CR_ATTRIBUTE_BASE_TYPE##n, n)

#define DEFINED_ATTRIBUTE(n) (defined(CR_ATTRIBUTE_BASE_TYPE##n) && defined(CR_ATTRIBUTE_COMPONENT_COUNT##n))
#define DECLARE_ATTRIBUTE(n) Buffer<CR_ATTRIBUTE_TYPE(n)> g_roVertexAtt##n

#define SET_TARGET(n) g_rwRenderTarget##n[pixelPos] = output.CR_TARGET##n
#define DEFINED_TARGET(n) (defined(CR_TARGET_TYPE##n) && defined(CR_TARGET##n))
#define DECLARE_TARGET(n) RWTexture2D<CR_TARGET_TYPE##n> g_rwRenderTarget##n

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
StructuredBuffer<TilePrim> g_roTilePrimitives;
#include "DeclareAttributes.hlsli"

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<float4> g_rwVertexPos;
#include "DeclareTargets.hlsli"
RWTexture2D<uint> g_rwDepth;
RWTexture2D<uint> g_rwHiZ;

[numthreads(8, 8, 1)]
void main(uint2 GTid : SV_GroupThreadID, uint Gid : SV_GroupID)//, uint GTidx : SV_GroupIndex)
{
	const TilePrim tilePrim = g_roTilePrimitives[Gid];
	const uint2 tile = uint2(tilePrim.TileIdx % g_tileDim.x, tilePrim.TileIdx / g_tileDim.x);

	float3x4 primVPos;

	// Load the vertex positions of the triangle
	const uint baseVIdx = tilePrim.PrimId * 3;
	[unroll]
	for (uint i = 0; i < 3; ++i) primVPos[i] = g_rwVertexPos[baseVIdx + i];

	// To screen space.
	ToScreenSpace(primVPos);

#if RE_HI_Z
	const uint zMin = asuint(min(primVPos[0].z, min(primVPos[1].z, primVPos[2].z)));
	if (g_rwHiZ[tile] < zMin) return;
#endif

	PSIn input;
	float3 w;
	const uint2 pixelPos = (tile << TILE_SIZE_LOG) + GTid;
	input.Pos.xy = pixelPos + 0.5;
	if (!Overlap(input.Pos.xy, (float3x2)primVPos, w)) return;

	// Normalize barycentric coordinates.
	const float area = determinant(primVPos[0].xy, primVPos[1].xy, primVPos[2].xy);
	if (area <= 0.0) return;
	w /= area;

	// Depth test
	uint depthMin;
	input.Pos.z = w.x * primVPos[0].z + w.y * primVPos[1].z + w.z * primVPos[2].z;
	const uint depth = asuint(input.Pos.z);
	InterlockedMin(g_rwDepth[pixelPos], depth, depthMin);
	if (depth > depthMin) return;

	// Interpolations
	float3 persp = float3(w.x * primVPos[0].w, w.y * primVPos[1].w, w.z * primVPos[2].w);
	input.Pos.w = 1.0 / (persp.x + persp.y + persp.z);
	persp *= input.Pos.w;
	
#include "SetAttributes.hlsli"

	// Call pixel shader
#ifndef CR_OUT_STRUCT_TYPE
#define CR_OUT_STRUCT_TYPE CR_TARGET_TYPE0
#endif
	CR_OUT_STRUCT_TYPE output = PSMain(input);

	// Mutual exclusive writing
	[allow_uav_condition]
	for (i = 0; i < 0xffffffff; ++i)
	{
		InterlockedExchange(g_rwDepth[pixelPos], 0xffffffff, depthMin);
		if (depthMin != 0xffffffff)
		{
			// Critical section
			if (depth <= depthMin)
			{
				//g_rwRenderTarget[pixelPos] = float4(w, 1.0);
#include "SetTargets.hlsli"
			}
			g_rwDepth[pixelPos] = min(depth, depthMin);
			break;
		}
	}
}
