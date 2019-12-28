//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define main VSMain
#include "VertexShader.hlsl"
#undef main

#define CR_ATTRIBUTE_GEN_TYPE(t, c) t##c
#define CR_ATTRIBUTE_TYPE(n) CR_ATTRIBUTE_GEN_TYPE(CR_ATTRIBUTE_BASE_TYPE##n, CR_ATTRIBUTE_COMPONENT_COUNT##n)

#define SET_ATTRIBUTE(n) g_rwVertexAtt##n[DTid] = output.CR_ATTRIBUTE##n

#define DEFINED_ATTRIBUTE(n) (defined(CR_ATTRIBUTE_BASE_TYPE##n) && defined(CR_ATTRIBUTE_COMPONENT_COUNT##n))
#define DECLARE_ATTRIBUTE(n) RWBuffer<CR_ATTRIBUTE_TYPE(n)> g_rwVertexAtt##n

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
StructuredBuffer<VSIn> g_roVertexBuffer;
Buffer<uint> g_roIndexBuffer;

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<float4> g_rwVertexPos;
#include "DeclareAttributes.hlsli"

//--------------------------------------------------------------------------------------
// Fetch shader
//--------------------------------------------------------------------------------------
void FetchShader(uint id, out VSIn result);

//--------------------------------------------------------------------------------------
// Vertex shader stage process
//--------------------------------------------------------------------------------------
[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	VSIn input;
	FetchShader(DTid, input);

	// Call vertex shader
	VSOut output = VSMain(input);

	g_rwVertexPos[DTid] = output.Pos;

#include "SetAttributes.hlsli"
}
