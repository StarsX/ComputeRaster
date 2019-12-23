//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define main VSMain
#include "VertexShader.hlsl"
#undef main

#define SET_ATTRIB(n) g_rwVertexAtt[n][DTid] = output.Att##n

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
StructuredBuffer<VSIn> g_roVertexBuffer;
Buffer<uint> g_roIndexBuffer;

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWBuffer<float4> g_rwVertexPos;
RWBuffer<float3> g_rwVertexAtt[ATTRIB_COUNT];

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

#if ATTRIB_COUNT > 0
	SET_ATTRIB(0);
#endif

#if ATTRIB_COUNT > 1
	SET_ATTRIB(1);
#endif

#if ATTRIB_COUNT > 2
	SET_ATTRIB(2);
#endif

#if ATTRIB_COUNT > 3
	SET_ATTRIB(3);
#endif

#if ATTRIB_COUNT > 4
	SET_ATTRIB(4);
#endif

#if ATTRIB_COUNT > 5
	SET_ATTRIB(5);
#endif

#if ATTRIB_COUNT > 6
	SET_ATTRIB(6);
#endif

#if ATTRIB_COUNT > 7
	SET_ATTRIB(7);
#endif
}
