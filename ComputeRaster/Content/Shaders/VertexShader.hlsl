//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct VSIn
{
	float3	Pos	: POSITION;
	float3	Nrm	: NORMAL;
};

struct VSOut
{
	float4 Pos	: SV_POSITION;
	float3 Nrm	: NORMAL;
};

cbuffer cb
{
	matrix g_worldViewProj;
	matrix g_normal;
};

//--------------------------------------------------------------------------------------
// Vertex shader
//--------------------------------------------------------------------------------------
VSOut main(VSIn input)
{
	VSOut output;

	output.Pos = float4(input.Pos, 1.0);
	output.Pos = mul(output.Pos, g_worldViewProj);
	output.Nrm = mul(input.Nrm, (float3x3)g_normal);

	return output;
}
