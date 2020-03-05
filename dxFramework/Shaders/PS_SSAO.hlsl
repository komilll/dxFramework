#include <PS_Input.hlsl> //PixelInputType

SamplerState baseSampler : register(s0);
Texture2D noiseTexture : register(t0);
Texture2D depthTexture : register(t1);
Texture2D normalTexture : register(t2);

cbuffer SpecialBufferSSAO : register(b13)
{
	float4 g_kernelSample[64];
	float4x4 g_projectionMatrix;
};

static const int g_sampleCount = 1;
static const float g_kernelRadius = 0.25f;
float4 main(PixelInputType input) : SV_TARGET
{
	const float2 noiseScale = float2(1280.0f / 4.0f, 720.0f / 4.0f);
	const float3 origin = input.viewRay.xyz * depthTexture.Sample(baseSampler, input.uv).r;
	const float3 normal = normalTexture.Sample(baseSampler, input.uv).xyz;
	
	const float3 randomVector = float3(noiseTexture.Sample(baseSampler, input.uv * noiseScale).xy, 0.0f);
	const float3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
	const float3 bitangent = cross(normal, tangent);	
	const matrix<float, 3, 3> TBN = {tangent, bitangent, normal};
	
	float occlussion = 0;
	
	for (int i = 0; i < g_sampleCount; ++i)
	{
		const float3 sample = mul(TBN, g_kernelSample[i].xyz) * g_kernelRadius + origin;
		
		float4 offset = float4(sample, 1.0f);
		offset = mul(g_projectionMatrix, offset);
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5f + 0.5f;
		
		const float sampleDepth = depthTexture.Sample(baseSampler, offset.xy).r;
		
		float rangeCheck = abs(origin.z - sampleDepth) < g_kernelRadius ? 1.0f : 0.0f;
		occlussion += (sampleDepth <= sample.z ? 1.0f : 0.0f) * rangeCheck;
	}
	
	occlussion = 1.0f - (occlussion / g_sampleCount);
	
	return float4(occlussion, occlussion, occlussion, 1.0f);	
}