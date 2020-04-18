#ifndef _PS_UNLIT_HLSL_
#define _PS_UNLIT_HLSL_

#include <All_UberBuffer.hlsl>
#include <LightSettings.hlsl>

struct PixelInputType
{
	float4 position : SV_POSITION;
};

SamplerState baseSampler : register(s0);
TextureCube<float4> skyboxTexture 	: register(t0);

float4 main(PixelInputType input) : SV_TARGET
{   
    return g_unlitColor;
}

#endif // !_PS_UNLIT_HLSL_