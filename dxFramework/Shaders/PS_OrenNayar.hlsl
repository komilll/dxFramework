#include <PS_Input.hlsl> //PixelInputType

float4 main(PixelInputType input) : SV_TARGET
{
	const float3 L = normalize(input.pointToLight.xyz);
	const float3 N = normalize(input.normal);
	const float intensity = input.pointToLight.w;	
	const float NdotL = dot(L, N);
	
	const float roughness2 = g_roughness * g_roughness;
	const float A = 1.0f - 0.5f * g_roughness / (roughness2 + 0.33f);
	const float B = 0.45f * roughness2 / (roughness2 + 0.09f);
	const float NdotV = dot(N, normalize(input.viewDir.xyz));
	const float theta_r = acos(NdotV);
	const float theta_i = acos(NdotL);
	const float cos_phi_diff = dot( normalize(input.viewDir.xyz * NdotV), normalize(input.pointToLight.xyz * NdotL) );
	const float alpha = max(theta_i, theta_r);
	const float beta = min(theta_i, theta_r);
	
	const float finalColor = NdotL * (A + (B * max(0, cos_phi_diff) * sin(alpha) * tan(beta)));
	
	return saturate(finalColor * intensity);	
}