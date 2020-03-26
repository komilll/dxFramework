#ifndef _PS_PRECOMPUTE_IBL_HLSL
#define _PS_PRECOMPUTE_IBL_HLSL

#include <PS_Input.hlsl> //PixelInputType
#include <ALL_UberBuffer.hlsl> //UberBuffer
#include <HammerslaySequence.hlsl>

const static float PI       = 3.14159265358979323846;
const static float INV_PI   = 0.31830988618379067154;
const static float INV_2PI  = 0.15915494309189533577;
const static float INV_4PI  = 0.07957747154594766788;

cbuffer PrecomputeIBLBuffer : register(b13)
{
    int g_currentCubemapFace;
    float3 g_paddingPrecomputeIBL;
}

SamplerState baseSampler : register(s0);
TextureCube<float4> skyboxTexture : register(t0);

#define PI 3.14159265f

float3 GetNormalVector(float2 uv)
{
    uv = uv * 2.0 - 1.0;
    switch (g_currentCubemapFace)
    {
        case 0:  return float3(   1.0,  -uv.y,  -uv.x ); //posX
        case 1:  return float3(  -1.0,  -uv.y,   uv.x ); //negX
        case 2:  return float3(  uv.x,    1.0,   uv.y ); //posY
        case 3:  return float3(  uv.x,   -1.0,  -uv.y ); //negY
        case 4:  return float3(  uv.x,  -uv.y,   1.0  ); //posZ
        default: return float3( -uv.x,  -uv.y,  -1.0  ); //negZ /* case 5 */
    }
}

float3 UniformSampleSphere(float u1, float u2)
{
    float z = 1.0 - 2.0 * u1;
    float r = sqrt(1.0 - z*z);
    float phi = 2.0 * PI * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);
    return float3(x, y, z);
}

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

float4 main(PixelInputType input) : SV_TARGET
{
    //https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/2.1.2.ibl_irradiance/2.1.2.irradiance_convolution.fs
    //input.position from VS_Skybox.hlsl
    // const float3 N = GetNormalVector(input.uv);

    // float3 irradiance = 0;
    // float3 up = float3(0, 1, 0);
    // float3 right = cross(up, N);
    // up = cross(N, right);

    // float sampleDelta = 0.025;
    // float samplesCount = 0;
    // for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    // {
    //     for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
    //     {
    //         //https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf#page=4&zoom=120,-297,352
    //         const float3 tangent = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    //         const float3 sampleVector = tangent.x * right + tangent.y * up + tangent.z * N;

    //         irradiance += skyboxTexture.Sample(baseSampler, sampleVector).rgb * cos(theta) * sin(theta);
    //         samplesCount++;
    //     }
    // }

    // return float4(PI * irradiance * (1.0f / float(samplesCount)), 1.0f);

    float3 irradiance = 0;

    const float3 N = normalize(GetNormalVector(input.uv));
    const float3 V = N;

    const uint sampleCount = 1024;
    for (uint i = 0; i < sampleCount; ++i)
    {
        float2 Xi = HammersleyDistribution(i, sampleCount);
        float3 L = UniformSampleSphere(Xi.x, Xi.y);
        [flatten]
        if (dot(L, N) < 0.0)
            L = -L;
        float NoL = dot(N, L);
        if (NoL > 0.0)
            irradiance += NoL * skyboxTexture.Sample(baseSampler, L).rgb / (INV_2PI * PI);
    }

    return float4(irradiance / (float) sampleCount, 1.0f);
}

#endif //_PS_PRECOMPUTE_IBL_HLSL