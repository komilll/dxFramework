#include <PS_Input.hlsl> //PixelInputType

SamplerState baseSampler : register(s0);
Texture2D baseTexture : register(t0);

float4 main(PixelInputType input) : SV_TARGET
{
	return baseTexture.Sample(baseSampler, input.uv);
	//return float4(1, 0, 0, 1);
}