#include <PS_Input.hlsl> //PixelInputType
#include <ALL_UberBuffer.hlsl>

float4 main(PixelInputType input) : SV_TARGET
{
	const float3 L = normalize(input.pointToLight.xyz);
	const float3 N = normalize(input.normal);
	const float intensity = input.pointToLight.w;	
	const float NdotL = dot(L, N);
	const float3 diffuse = NdotL;
	const float3 H = normalize(L + input.viewDir.xyz);
	const float3 specular = pow(max(dot(N, H), 0.0f), 32.0f) * 0.1f;
	
	return float4(saturate((diffuse + specular) * intensity * g_directionalLightColor.xyz), 1.0f);	
}