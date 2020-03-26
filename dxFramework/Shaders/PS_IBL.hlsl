#ifndef _PS_IBL_HLSL_
#define _PS_IBL_HLSL_

#include <HammerslaySequence.hlsl>

SamplerState baseSampler : register(s0);
TextureCube<float4> specularReflectanceTexture : register(t4);

#define PI 3.14159265f

float BitwiseInverseInRange(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // 1.0f / 2^32
}

float2 HammersleyDistribution(uint index, uint sampleCount)
{
    return float2(float(index) / float(sampleCount), BitwiseInverseInRange(index));
}

float3 ImportanceSamplingGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness*roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    float3 H = 0;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    float3 up        = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0f) : float3(1.0f, 0.0, 0.0);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
	
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float3 SpecularIBL(float roughness, float3 N)
{
    float3 specularLighting = 0;
    int mipLevel = 0;
    float weightSum = 0;
    const float3 V = N;

    const int sampleCount = 1024;
    for (uint i = 0; i < sampleCount; ++i)
    {
        float2 Xi = HammersleyDistribution(i, sampleCount);
        // float2 Xi = hammerslaySequence[i];
        float3 H = ImportanceSamplingGGX(Xi, N, roughness);
        float3 L = 2 * dot(V, H) * H - V;

        float NoL = max(dot(V, L), 0);
        if (NoL > 0)
        {
            specularLighting += specularReflectanceTexture.SampleLevel(baseSampler, L, mipLevel).xyz * NoL;    
            weightSum += NoL;        
        }
    }
    
    return specularLighting / weightSum;
}

#endif //_PS_IBL_HLSL_