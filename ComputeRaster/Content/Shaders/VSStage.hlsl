//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define main VSMain
#include "VertexShader.hlsl"
#undef main

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
StructuredBuffer<VSIn> g_roVertexBuffer;

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWBuffer<float4> g_rwVertexPos;

//--------------------------------------------------------------------------------------
// Fetch shader
//--------------------------------------------------------------------------------------
void FetchShader(uint id, out VSIn result)
{
	result = g_roVertexBuffer[id];
}

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	VSIn input;
	FetchShader(DTid, input);

	// Call vertex shader
	VSOut output = VSMain(input);

	g_rwVertexPos[DTid] = output.Pos;
}
