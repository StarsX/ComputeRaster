//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structure
//--------------------------------------------------------------------------------------
struct TilePrim
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
void ToScreenSpace(inout float3x4 primVPos)
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

//--------------------------------------------------------------------------------------
// Move the vertex by the pixel bias.
//--------------------------------------------------------------------------------------
float2 Scale(float2 pv, float2 cv, float2 nv, float pixelBias = 0.5)
{
	float3 plane0 = cross(float3(cv - pv, 0.0), float3(pv, 1.0));
	float3 plane1 = cross(float3(nv - cv, 0.0), float3(cv, 1.0));

	plane0.z -= dot(pixelBias, abs(plane0.xy));
	plane1.z -= dot(pixelBias, abs(plane1.xy));

	const float3 result = cross(plane0, plane1);

	return result.xy / result.z;
}

//--------------------------------------------------------------------------------------
// Scale the primitive vertices by the pixel bias.
//--------------------------------------------------------------------------------------
float3x2 Scale(float3x2 v, float pixelBias)
{
	float3x2 sv;
	sv[0] = Scale(v[2], v[0], v[1], pixelBias);
	sv[1] = Scale(v[0], v[1], v[2], pixelBias);
	sv[2] = Scale(v[1], v[2], v[0], pixelBias);

	return sv;
}

//--------------------------------------------------------------------------------------
// Check if the point is overlapped by a primitive.
//--------------------------------------------------------------------------------------
float3 ComputeUnnormalizedBarycentric(float2 pos, float3x2 n, float2 minPt, float3 w)
{
	const float2 disp = pos - minPt;
	w += mul(n, disp);

	return w;
}

//--------------------------------------------------------------------------------------
// Check if the point is overlapped by a primitive.
//--------------------------------------------------------------------------------------
bool Overlap(float2 pos, float3x2 n, float2 minPt, float3 w)
{
	w = ComputeUnnormalizedBarycentric(pos, n, minPt, w);

	return all(w >= 0.0);
}

//--------------------------------------------------------------------------------------
// Check if the point is overlapped by a primitive.
//--------------------------------------------------------------------------------------
bool Overlap(float2 pos, float3x2 v, out float3 w)
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
	w = ComputeUnnormalizedBarycentric(pos, n, minPt, w);

	return all(w >= 0.0);
}
