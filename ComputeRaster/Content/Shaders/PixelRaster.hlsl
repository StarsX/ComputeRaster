//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"
#define main PSMain
#include "PixelShader.hlsl"
#undef main

#define COMPUTE_ATTRIB(n) \
	[unroll] \
	for (i = 0; i < 3; ++i) primVAtt[i] = g_roVertexAtt[n][baseVIdx + i]; \
	[unroll] \
	for (i = 0; i < 3; ++i) primVAtt[i] *= primVPos[i].w; \
	att = w.x * primVAtt[0] + w.y * primVAtt[1] + w.z * primVAtt[2]; \
	input.Att##n = att * input.Pos.w;

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
Buffer<float4> g_roVertexPos;
StructuredBuffer<TiledPrim> g_roTiledPrimitives;
Buffer<float3> g_roVertexAtt[ATTRIB_COUNT];

//--------------------------------------------------------------------------------------
// UAV buffers
//--------------------------------------------------------------------------------------
RWTexture2D<float4>	g_rwRenderTarget;
RWTexture2D<uint>	g_rwDepth;

//--------------------------------------------------------------------------------------
// Check if the tile is overlapped by a straight line with a certain thickness.
//--------------------------------------------------------------------------------------
bool Overlap(float2 pixelPos, float4 v[3], out float3 w)
{
	// Triangle edge equation setup.
	const float3x2 n =
	{
		v[1].y - v[2].y, v[2].x - v[1].x,
		v[2].y - v[0].y, v[0].x - v[2].x,
		v[0].y - v[1].y, v[1].x - v[0].x,
	};

	// Calculate barycentric coordinates at min corner.
	const float2 minPt = min(v[0].xy, min(v[1].xy, v[2].xy));
	w.x = determinant(v[1].xy, v[2].xy, minPt);
	w.y = determinant(v[2].xy, v[0].xy, minPt);
	w.z = determinant(v[0].xy, v[1].xy, minPt);

	// If pixel is inside of all edges, set pixel.
	const float2 disp = pixelPos - minPt;
	w += mul(n, disp);

	return w.x >= 0.0 && w.y >= 0.0 && w.z >= 0.0;
}

[numthreads(8, 8, 1)]
void main(uint2 GTid : SV_GroupThreadID, uint Gid : SV_GroupID)//, uint GTidx : SV_GroupIndex)
{
	const TiledPrim tiledPrim = g_roTiledPrimitives[Gid];
	const uint2 tile = uint2(tiledPrim.TileIdx % g_tileDim.x, tiledPrim.TileIdx / g_tileDim.x);

	float4 primVPos[3];

	// Load the vertex positions of the triangle
	const uint baseVIdx = tiledPrim.PrimId * 3;
	[unroll]
	for (uint i = 0; i < 3; ++i) primVPos[i] = g_roVertexPos[baseVIdx + i];

	// To screen space.
	ToScreenSpace(primVPos);

	PSIn input;
	float3 w;
	const uint2 pixelPos = (tile << 3) + GTid;
	input.Pos.xy = pixelPos + 0.5;
	if (!Overlap(input.Pos.xy, primVPos, w)) return;

	// Normalize barycentric coordinates.
	const float area = determinant(primVPos[0].xy, primVPos[1].xy, primVPos[2].xy);
	if (area <= 0.0) return;
	w /= area;

	// Depth test
	uint depthMin;
	input.Pos.z = w.x * primVPos[0].z + w.y * primVPos[1].z + w.z * primVPos[2].z;
	const uint depth = asuint(input.Pos.z);
	InterlockedMin(g_rwDepth[pixelPos], asuint(input.Pos.z), depthMin);
	if (depth > depthMin) return;

	// Interpolations
	const float rhw = w.x * primVPos[0].w + w.y * primVPos[1].w + w.z * primVPos[2].w;
	input.Pos.w = 1.0 / rhw;

	float3 primVAtt[3], att;
#if ATTRIB_COUNT > 0
	COMPUTE_ATTRIB(0);
#endif

#if ATTRIB_COUNT > 1
	COMPUTE_ATTRIB(1);
#endif

#if ATTRIB_COUNT > 2
	COMPUTE_ATTRIB(2);
#endif

#if ATTRIB_COUNT > 3
	COMPUTE_ATTRIB(3);
#endif

#if ATTRIB_COUNT > 4
	COMPUTE_ATTRIB(4);
#endif

#if ATTRIB_COUNT > 5
	COMPUTE_ATTRIB(5);
#endif

#if ATTRIB_COUNT > 6
	COMPUTE_ATTRIB(6);
#endif

#if ATTRIB_COUNT > 7
	COMPUTE_ATTRIB(7);
#endif

	//g_rwRenderTarget[pixelPos] = float4(w, 1.0);
	g_rwRenderTarget[pixelPos] = PSMain(input);	// Call pixel shader
}
