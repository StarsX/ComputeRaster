//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct PSIn
{
	float4	Pos	: SV_POSITION;
	float3	Nrm	: NORMAL;
};

cbuffer cbLighting
{
	float4 g_ambientColor;
	float4 g_lightColor;
	float3 g_lightPt;
};

cbuffer cbMaterial
{
	float3 g_baseColor;
};

static float3 g_ambient = g_ambientColor.xyz * g_ambientColor.w;
static float3 g_light = g_lightColor.xyz * g_lightColor.w;

//--------------------------------------------------------------------------------------
// Vertex shader
//--------------------------------------------------------------------------------------
float4 main(PSIn input) : SV_TARGET
{
	const float3 L = normalize(g_lightPt);
	const float3 N = normalize(input.Nrm);

	const float lightAmt = saturate(dot(N, L));
	const float ambientAmt = N.y * 0.5 + 0.5;

	const float3 diffuse = lightAmt * g_light;
	const float3 ambient = ambientAmt * g_ambient;

	return float4(g_baseColor * (diffuse + ambient), 1.0);
}
