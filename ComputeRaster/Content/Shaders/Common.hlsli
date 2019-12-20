//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structure
//--------------------------------------------------------------------------------------
struct TiledPrim
{
	uint TileIdx;
	uint PrimId;
};

//--------------------------------------------------------------------------------------
// Constant buffer
//--------------------------------------------------------------------------------------
cbuffer cb
{
	float4	g_viewport;	// X, Y, W, H
	uint2	g_tileDim;
};

//--------------------------------------------------------------------------------------
// Transform a vector in homogeneous clip space to the screen space.
// --> First does perspective division to get the normalized device coordinates.
// --> Then scales the coordinates to the whole screen.
//--------------------------------------------------------------------------------------
float4 ClipToScreen(float4 pos)
{
	const float rhw = 1.0 / pos.w;
	pos.xyz *= rhw;
	pos.y = -pos.y;
	pos.xy = pos.xy * 0.5 + 0.5;
	pos.xy = pos.xy * g_viewport.zw;

	return float4(pos.xyz, rhw);
}

//--------------------------------------------------------------------------------------
// Transform a primitive given in clip space to screen space.
//--------------------------------------------------------------------------------------
void ToScreenSpace(inout float4 primVPos[3])
{
	[unroll]
	for (uint i = 0; i < 3; ++i)
		primVPos[i] = ClipToScreen(primVPos[i]);
}

//--------------------------------------------------------------------------------------
// Determinant
//--------------------------------------------------------------------------------------
float determinant(float2 a, float2 b, float2 c)
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}
