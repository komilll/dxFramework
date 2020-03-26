#include <All_UberBuffer.hlsl>

struct PixelInputType
{
	float4 position : SV_POSITION;
    float3 positionWS : TEXCOORD0;
};

SamplerState baseSampler : register(s0);
TextureCube<float4> skyboxTexture 	: register(t0);

float4 main(PixelInputType input) : SV_TARGET
{
    return skyboxTexture.Sample(baseSampler, normalize(input.positionWS - g_viewerPosition.xyz) * float3(1,1,-1));
}