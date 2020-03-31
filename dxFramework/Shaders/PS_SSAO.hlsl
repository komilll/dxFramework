#include <PS_Input.hlsl> //PixelInputType

SamplerState baseSampler : register(s0);
Texture2D noiseTexture : register(t0);
Texture2D positionTexture : register(t1);
Texture2D normalTexture : register(t2);

cbuffer SpecialBufferSSAO : register(b13)
{
	float4 g_kernelSample[64];
	float4x4 g_projectionMatrix;
	int g_sampleCount;
	float g_kernelRadius;
	float2 g_paddingSpecialBufferSSAO;
};

static const float bias = 0.025f;
static const float2 noiseScale = float2(1280.0f / 4.0f, 720.0f / 4.0f);

float4 ComputeSSAO(float3 origin, matrix<float, 3, 3> TBN)
{
	float occlussion = 0;	

	for (int i = 0; i < g_sampleCount; ++i)
	{					
		float3 sample = mul(TBN, g_kernelSample[i].xyz);		
		sample = origin + sample * g_kernelRadius;
		
		float4 offset = float4(sample, 1.0f);
		offset = mul(g_projectionMatrix, offset);
		offset.xy /= offset.w;		
		offset.xy = offset.xy * 0.5f + 0.5f;
				
		const float sampleDepth = positionTexture.Sample(baseSampler, offset.xy).z;
		const float rangeCheck = abs(origin.z - sampleDepth) < g_kernelRadius ? 1.0f : 0.0f;
		
		occlussion += (sampleDepth <= sample.z + bias ? 1.0f : 0.0f) * rangeCheck;		
	}
	
	occlussion = 1.0f - (occlussion / g_sampleCount);
	
	return float4(occlussion, occlussion, occlussion, 1.0f);
}

float4 main(PixelInputType input) : SV_TARGET
{	
	const float3 origin = positionTexture.Sample(baseSampler, input.uv).xyz;	
	const float3 normal = normalize(normalTexture.Sample(baseSampler, input.uv).xyz * 2.0f - 1.0f);

	const float3 randomVector = float3(noiseTexture.Sample(baseSampler, input.uv * noiseScale).xy, 0.0f) * 2.0f - 1.0f;
	const float3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
	const float3 bitangent = cross(normal, tangent);		
	const matrix<float, 3, 3> TBN = {normal, bitangent, tangent};	

	return ComputeSSAO(origin, TBN);
}