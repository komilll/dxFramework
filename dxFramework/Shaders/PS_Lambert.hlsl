#include <PS_Input.hlsl> //PixelInputType

SamplerState baseSampler : register(s0);
Texture2D baseTexture : register(t0);

float4 main(PixelInputType input) : SV_TARGET
{		
	const float3 L = normalize(input.pointToLight.xyz);
	const float3 N = normalize(input.normal);
	const float intensity = input.pointToLight.w;	
	const float NdotL = dot(L, N);
	const float ambient = 0.01f;
	const float brightness = saturate(NdotL + ambient);
	
	//Gamma correction of albedo
	float3 albedo = baseTexture.Sample(baseSampler, input.uv);
	albedo = albedo / (albedo + float3(1.0, 1.0, 1.0));
	albedo = pow(albedo, float3(1.0/2.2, 1.0/2.2, 1.0/2.2)); 
		
	const float3 finalColor = brightness * intensity * albedo;

	return float4(finalColor * g_directionalLightColor, 1.0f);	
}