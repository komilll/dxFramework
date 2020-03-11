#ifndef _PS_INPUT_HLSL_
#define _PS_INPUT_HLSL_

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv: TEXCOORD0;
	float4 pointToLight : TEXCOORD1;
	float4 viewDir : TEXCOORD2;
	float4 viewRay : TEXCOORD3;
};

cbuffer PropertyBuffer : register(b0)
{
	float g_roughness;	
	float3 g_directionalLightColor;	
};

#endif //_PS_INPUT_HLSL_