#include <PS_Input.hlsl> //PixelInputType

SamplerState baseSampler : register(s0);
Texture2D ssaoTexture : register(t0);

static const float2 texelSize = 1.0f / float2(1280.0f, 720.0f);
float4 main(PixelInputType input) : SV_TARGET
{	
	float result = 0.0f;
	for (float x = -2; x < 2; ++x)
	{
		for (float y = -2; y < 2; ++y)
		{
			float2 offset = float2(x, y) * texelSize;
			result += ssaoTexture.Sample(baseSampler, input.uv + offset).r;
		}
	}
	result *= (1.0f / 16.0f);
	
	return float4(result, result, result, 1.0f);
}