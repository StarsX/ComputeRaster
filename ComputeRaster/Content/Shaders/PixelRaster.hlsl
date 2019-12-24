//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"
#define main PSMain
#include "PixelShader.hlsl"
#undef main

#define CR_PRIMITIVE_VERTEX_ATTRIBUTE_TYPE(t, c) t##3x##c
#define CR_ATTRIBUTE_TYPE(t, c) t##c
#define CR_ATTRIBUTE_FORMAT(n) CR_ATTRIBUTE_TYPE(CR_ATTRIBUTE_BASE_TYPE##n, CR_ATTRIBUTE_COMPONENT_COUNT##n)

#define COMPUTE_ATTRIBUTE_float(n) \
	{ \
		CR_PRIMITIVE_VERTEX_ATTRIBUTE_TYPE(CR_ATTRIBUTE_BASE_TYPE##n, CR_ATTRIBUTE_COMPONENT_COUNT##n) primVAtt; \
		CR_ATTRIBUTE_FORMAT(n) attribute; \
		[unroll] \
		for (i = 0; i < 3; ++i) primVAtt[i] = g_roVertexAtt##n[baseVIdx + i]; \
		[unroll] \
		for (i = 0; i < 3; ++i) primVAtt[i] *= primVPos[i].w; \
		attribute = mul(w, primVAtt); \
		input.CR_ATTRIBUTE##n = attribute * input.Pos.w; \
	}

#define COMPUTE_ATTRIBUTE_min16float(n) COMPUTE_ATTRIBUTE_float(n)
#define COMPUTE_ATTRIBUTE_int(n) input.CR_ATTRIBUTE##n = g_roVertexAtt##n[baseVIdx]
#define COMPUTE_ATTRIBUTE_uint(n) COMPUTE_ATTRIBUTE_int(n)
#define COMPUTE_ATTRIBUTE_min16int(n) COMPUTE_ATTRIBUTE_int(n)
#define COMPUTE_ATTRIBUTE_min16uint(n) COMPUTE_ATTRIBUTE_int(n)
#define COMPUTE_ATTRIBUTE_bool(n) COMPUTE_ATTRIBUTE_int(n)

#define COMPUTE_ATTRIBUTE(t, n) COMPUTE_ATTRIBUTE_##t(n)
#define SET_ATTRIBUTE(n) COMPUTE_ATTRIBUTE(CR_ATTRIBUTE_BASE_TYPE##n, n)

#define DEFINED(n) defined(CR_ATTRIBUTE_BASE_TYPE##n) && defined(CR_ATTRIBUTE_COMPONENT_COUNT##n)
#define DECLARE_ATTRIBUTE(n) Buffer<CR_ATTRIBUTE_FORMAT(n)> g_roVertexAtt##n

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
Buffer<float4> g_roVertexPos;
StructuredBuffer<TilePrim> g_roTilePrimitives;
#include "DeclareAttributes.hlsli"

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWTexture2D<float4>	g_rwRenderTarget;
RWTexture2D<uint>	g_rwDepth;

[numthreads(8, 8, 1)]
void main(uint2 GTid : SV_GroupThreadID, uint Gid : SV_GroupID)//, uint GTidx : SV_GroupIndex)
{
	const TilePrim tilePrim = g_roTilePrimitives[Gid];
	const uint2 tile = uint2(tilePrim.TileIdx % g_tileDim.x, tilePrim.TileIdx / g_tileDim.x);

	float3x4 primVPos;

	// Load the vertex positions of the triangle
	const uint baseVIdx = tilePrim.PrimId * 3;
	[unroll]
	for (uint i = 0; i < 3; ++i) primVPos[i] = g_roVertexPos[baseVIdx + i];

	// To screen space.
	ToScreenSpace(primVPos);

	PSIn input;
	float3 w;
	const uint2 pixelPos = (tile << 3) + GTid;
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
	const float rhw = w.x * primVPos[0].w + w.y * primVPos[1].w + w.z * primVPos[2].w;
	input.Pos.w = 1.0 / rhw;
	
#include "SetAttributes.hlsli"

	//g_rwRenderTarget[pixelPos] = float4(w, 1.0);
	g_rwRenderTarget[pixelPos] = PSMain(input);	// Call pixel shader
}
