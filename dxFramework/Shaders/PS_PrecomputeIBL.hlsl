#ifndef _PS_PRECOMPUTE_IBL_HLSL
#define _PS_PRECOMPUTE_IBL_HLSL

#include <PS_Input.hlsl> //PixelInputType
#include <ALL_UberBuffer.hlsl> //UberBuffer
#include <HammerslaySequence.hlsl>
#include <PS_BRDF_Helper.hlsl>
#include <LightSettings.hlsl>

SamplerState baseSampler : register(s0);
TextureCube<float4> skyboxTexture      : register(t4);
TextureCube<float4> diffuseIBLTexture  : register(t5);
TextureCube<float4> specularIBLTexture : register(t6);
Texture2D           enviroBRDF         : register(t7);

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer PrecomputeIBLBuffer : register(b13)
{
    int g_currentCubemapFace;
    float g_roughnessIBL;
    float2 g_paddingPrecomputeIBL;
}

cbuffer FrameInfoBuffer : register(b1)
{
    float2 g_renderTargetSize;
    float g_frame;
    float g_frameInfoBufferPadding;
}

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
    float z = 1.0 - 2.0 * u1; //u1 = [0, 1]; z = [-1, 1]
    float r = sqrt(1.0 - z*z);
    float phi = 2.0 * PI * u2; //u2 = [0, 1] - low-discrepancy
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

float2 HammersleyDistribution(uint i, uint N, uint2 random)
{
    // Transform to 0-1 interval (2.3283064365386963e-10 == 1.0 / 2^32)
#if 0
    float E1 = frac(float(i)/float(N) + float(random.x & 0xffff) / (1<<16));
    float E2 = float(reversebits(i) ^ random.y) * 2.3283064365386963e-10; // / 0x100000000
    return float2(E1, E2);
#else
    float2 u = float2(float(i) / float(N), reversebits(i) * 2.3283064365386963e-10);
    u = frac(u + float2(random & 0xffff) / (1 << 16));
    return u;
#endif
}


// [Tea, a Tiny Encryption Algorithm]
// [http://citeseer.ist.psu.edu/viewdoc/download?doi=10.1.1.45.281&rep=rep1&type=pdf]
// [http://gaim.umbc.edu/2010/07/01/gpu-random-numbers/]
uint2 randomTea(uint2 seed, uint iterations = 8)
{
    const uint k[4] = { 0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E };
    const uint delta = 0x9E3779B9;
    uint sum = 0;
    [unroll]
    for (uint i = 0; i < iterations; ++i)
    {
        sum += delta;
        seed.x += (seed.x << 4) + k[0] ^ seed.x + sum ^ (seed.x >> 5) + k[1];
        seed.y += (seed.y << 4) + k[2] ^ seed.y + sum ^ (seed.y >> 5) + k[3];
    }
    return seed;
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

bool IntersectLight(float3 rayOrigin, float3 rayDirection, float3 lightPosition, float radiusSquared, out float t)
{
    float3 d = rayOrigin - lightPosition;
    float b = dot(rayDirection, d);
    float c = dot(d, d) - radiusSquared;
    t = b * b - c;
    if (t > 0.0)
    {
        t = -b - sqrt(t);
        return t > 0.0;
    }
    return true;
}

float4 MonteCarloSpecular(float3 positionWS, float3 diffuseColor, float3 specularColor, float3 N, float3 V, float roughness, const uint sampleCount, Light light)
{
    float3 specularLighting = 0;
    
    uint lightCount;
    uint stride;
    
    float3 delta = light.positionWS - positionWS;
    float distanceSquared = dot(delta, delta);
    if (distanceSquared <= light.radius * light.radius)
    {
        return float4(1, 0, 1, 0);
    }
    
    float4 positionCS = mul(float4(positionWS, 1.0), mul(viewMatrix, projectionMatrix));
    float2 uv = (positionCS.xy / positionCS.w) * 0.5 + 0.5;
    uint2 seed = randomTea(asuint(uv * g_renderTargetSize) + g_frame * float2(3, 8), 8);
    float t;
    
    for (uint i = 0; i < sampleCount; ++i)
    {
        float2 Xi = HammersleyDistribution(i, sampleCount, seed);
        float3 L = UniformSampleSphere(Xi.x, Xi.y);
        
        [flatten]
        if (dot(N, L) < 0.0)
            L = -L;
        
        if (IntersectLight(positionWS, L, light.positionWS, light.radius * light.radius, t))
        {
            const float3 H = normalize(V + L);
            const float NoL = saturate(dot(N, L));
            const float3 R = normalize(2.0 * NoL * N - L);
            const float NoV = saturate(dot(N, V));
            const float NoH = saturate(dot(N, H));
            const float VoR = saturate(dot(V, R));
            const float VoH = saturate(dot(V, H));
            const float VoL = saturate(dot(V, L));
            const float LoH = saturate(dot(L, H));
        
            //const float3 diffuse = Diffuse_Disney(NoV, NoL, LoH, roughness) * diffuseColor;
            const float D = Specular_D_GGX(roughness, NoH);
            const float G = Specular_G_GGX(roughness, NoV) * Specular_G_GGX(roughness, NoL);
            const float3 F = Specular_F_Schlick(specularColor, VoH);
            const float3 specular = F * (D * G);
        
            specularLighting += (specular / INV_2PI);
        }
    }
    return float4(specularLighting / sampleCount, 1.0f);
}

float4 SpecularPrecomputeIBL(float3 R)
{
    float3 specularLighting = 0;
    float weightSum = 0;
    const float3 V = R;
    const float3 N = R;

    const uint sampleCount = 1024;
    for (uint i = 0; i < sampleCount; ++i)
    {
        float2 Xi = HammersleyDistribution(i, sampleCount);
        float3 H = ImportanceSamplingGGX(Xi, N, g_roughnessIBL);
        float3 L = 2 * dot(V, H) * H - V;
        const float NoL = max(dot(V, L), 0);
        if (NoL > 0)
        {
            specularLighting += skyboxTexture.SampleLevel(baseSampler, L, 0).xyz * NoL;    
            weightSum += NoL;        
        }
    }
    return float4(specularLighting / weightSum, 1.0f);
}

float4 SpecularPrecompute(PixelInputType input) : SV_TARGET
{
    const float3 R = normalize(GetNormalVector(input.uv));
    return SpecularPrecomputeIBL(R);
}

float4 DiffusePrecomputeIBL(float3 N)
{
    float3 irradiance = 0;
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

float4 DiffusePrecompute(PixelInputType input) : SV_TARGET
{
    const float3 N = normalize(GetNormalVector(input.uv));
    return DiffusePrecomputeIBL(N);
}

float4 DiffuseIrradianceByLearnOpenGL(PixelInputType input) : SV_TARGET
{
    // Similar results to hammerslay sequence but brighter pixels provide too much contribution
    // Also areas in the middle of texture are not correctly sampled, introduce 1-pixel dot in the middle
    // or doesn't take into account other pixels as much as they should
    // https://learnopengl.com/PBR/IBL/Diffuse-irradiance
    
    return float4(0, 0, 0, 1);
}

float Specular_G(float roughness, float NoV)
{
    const float a = roughness * roughness;
    const float k = a / 2.0f;
    return NoV / (NoV * (1.0f - k) + k);
}

float2 PrecomputeEnvironmentLUTFin(float NoV, float roughness)
{
    const float3 V = float3(sqrt(1.0 - NoV * NoV), 0, NoV);
    const float3 N = float3(0, 0, 1.0);
    float A = 0;
    float B = 0;
    
    const uint sampleCount = 8096;
    for (uint i = 0; i < sampleCount; ++i)
    {
        float2 Xi = HammersleyDistribution(i, sampleCount);
        float3 H = ImportanceSamplingGGX(Xi, N, roughness);
        float3 L = 2.0 * dot(V, H) * H - V;
        
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        const float VoH = saturate(dot(V, H));
        
        if (NoL > 0)
        {
            float G1 = Specular_G(roughness, NoV);
            float G2 = Specular_G(roughness, NoL);
            float G = G1 * G2;
            
            G = (G * VoH) / (NoH * NoV);
            float Fc = pow(1.0 - VoH, 5.0);
            A += (1.0 - Fc) * G;
            B += Fc * G;
        }
    }
    return float2(A, B) / sampleCount;
}

float4 PrecomputeEnvironmentLUT(PixelInputType input) : SV_TARGET
{
    return float4(PrecomputeEnvironmentLUTFin(input.uv.x, input.uv.y), 0, 1);
}

#endif //_PS_PRECOMPUTE_IBL_HLSL