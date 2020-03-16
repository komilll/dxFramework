#include <PS_Input.hlsl> //PixelInputType
#include <ALL_SettingsBRDF.hlsl>
#include <CBuffer_BindingsBRDF.hlsl>

SamplerState baseSampler : register(s0);

Texture2D albedoTexture 	: register(t0);
Texture2D roughnessTexture  : register(t1);
Texture2D normalTexture 	: register(t2);
Texture2D metallicTexture 	: register(t3);

#define PI 3.14159265f
#define invPI 1.0f / 3.14159265f

//Disney diffuse function
float Diffuse_Disney(float NoV, float NoL, float LoH, float roughness)
{
	float fd90 = 0.5f + 2 * roughness * LoH * LoH;
	return invPI * (1.0f+(fd90-1.0f)*pow(1.0f-NoL, 5)) * (1.0f+(fd90-1.0f)*pow(1.0f-NoV, 5));
}

//Normal distribution functions
float Specular_D_Beckmann(float roughness, float NoH)
{
    const float a2 = roughness * roughness;
    const float NoH2 = NoH * NoH;
    return exp( (NoH2-1.0f) / (a2*NoH2) ) / (a2*PI*NoH2*NoH2);
}

float Specular_D_GGX(float roughness, float NoH)
{
    const float a2 = roughness * roughness;
    const float NoH2 = NoH * NoH;
    return a2 / (PI * pow((NoH2*(a2-1)+1), 2) );
}

float Specular_D_Blinn_Phong(float roughness, float NoH)
{
    const float a2 = roughness * roughness;
    const float NoH2 = NoH * NoH;
    return pow(NoH, (2/a2-2)) / (PI*a2);
}

//Geometry shadowing functions
float Specular_G_Implicit(float NoL, float NoV)
{
	return NoL * NoV;
}

float Specular_G_Neumann(float NoL, float NoV)
{
	return (NoL * NoV) / max(NoL, NoV);
}

float Specular_G_CT(float NoH, float NoV, float NoL, float VoH)
{
	return min( min(1.0f, (2.0f*NoH*NoV/VoH)), (2.0f*NoH*NoL/VoH) );
}

float Specular_G_Kelemen(float NoV, float NoL, float VoH)
{
	return NoL*NoV/(VoH*VoH);
}

float Specular_G_Beckmann(float NoV, float roughness)
{
	const float c = NoV / (roughness * sqrt(1 - NoV*NoV));
	if (c >= 1.6f)
		return 1.0f;
	else
		return (3.535f*c + 2.181f*c*c) / (1.0f + 2.276f*c + 2.577f*c*c);
}

float Specular_G_GGX(float roughness, float NoV)
{
	const float a2 = roughness * roughness;
	const float NoV2 = NoV * NoV;
	return (2.0f*NoV) / (NoV + sqrt(a2+(1-a2)*NoV2));
}

float Specular_G_SchlickBeckmann(float roughness, float NoV)
{
	const float k = roughness * sqrt(2.0f/PI);
	return NoV / (NoV*(1-k) + k);
}

float Specular_G_SchlickGGX(float roughness, float NoV)
{
	const float k = roughness/2.0f;
	return NoV / (NoV*(1.0f-k) + k);
}

//Fresnel functions
float Specular_F_Schlick(float f0, float NoH)
{
	return f0 + (1-f0)*pow(1-NoH, 5);
}

float Specular_F_CT(float f0, float NoH)
{
	const float f0Sqrt = sqrt(f0);
	const float eta = (1+f0Sqrt)/(1-f0Sqrt);
	const float g = sqrt(eta*eta + NoH*NoH - 1);
	const float c = NoH;

	return 0.5f * pow((g-c)/(g+c), 2) * pow((1 + ((g+c)*c-1) / ((g-c)*c + 1)), 2);
}

float4 main(PixelInputType input) : SV_TARGET
{	
	const float roughness = g_hasRoughness == 0 ? g_roughnessValue : roughnessTexture.Sample(baseSampler, input.uv).r;
	const float metallic = g_hasMetallic == 0 ? g_metallicValue : metallicTexture.Sample(baseSampler, input.uv).r;

	float3 N;
	if (g_hasNormal > 0){
		const matrix<float, 3, 3> TBN = { normalize(input.tangent), normalize(input.binormal), normalize(input.normal) };	
		N = normalTexture.Sample(baseSampler, input.uv).rgb;
		N = normalize(N * 2.0f - 1.0f);
		N = normalize(mul(TBN, N));
	}
	else{
		N = normalize(input.normal);
	}

	const float3 L = normalize(input.pointToLight.xyz);
	const float intensity = input.pointToLight.w;	
	const float NoL = dot(L, N);
	const float3 diffuse = NoL;
	const float3 H = normalize(L + input.viewDir.xyz);
	const float3 specular = pow(max(dot(N, H), 0.0f), 32.0f) * 0.1f;
	const float NoH = dot(N, H);
	const float NoV = dot(N, input.viewDir.xyz);
	const float LoH = dot(L, H);
	const float VoH = dot(input.viewDir.xyz, H);

	//float3 albedo = albedoTexture.Sample(baseSampler, input.uv);
	float3 albedo = float3(1.0f, 0.0f, 0.0f);
		
	//D component
	float D = 1.0f;
	if (g_ndfType == NDF_BECKMANN){
		D = Specular_D_Beckmann(roughness, NoH);
	}
	if (g_ndfType == NDF_GGX){
		D = Specular_D_GGX(roughness, NoH);
	}
	if (g_ndfType == NDF_BLINN_PHONG){
		D = Specular_D_Blinn_Phong(roughness, NoH);
	}
	D = saturate(D);

	//G component
	float G = 1.0f;
	if (g_geometryType == GEOM_IMPLICIT){
		G = Specular_G_Implicit(NoL, NoV);
	}
	if (g_geometryType == GEOM_NEUMANN){
		G = Specular_G_Neumann(NoL, NoV);
	}
	if (g_geometryType == GEOM_COOK_TORRANCE){
		G = Specular_G_CT(NoH, NoV, NoL, VoH);
	}
	if (g_geometryType == GEOM_KELEMEN){
		G = Specular_G_Kelemen(NoV, NoL, VoH);
	}
	if (g_geometryType == GEOM_BECKMANN){
		G = Specular_G_Beckmann(NoV, roughness);
	}
	if (g_geometryType == GEOM_GGX){
		G = Specular_G_GGX(roughness, NoV);
	}
	if (g_geometryType == GEOM_SCHLICK_BECKMANN){
		G = Specular_G_SchlickBeckmann(roughness, NoV);
	}
	if (g_geometryType == GEOM_SCHLICK_GGX){
		G = Specular_G_SchlickGGX(roughness, NoV);
	}
	G = saturate(G);

	//F component
	float F = 0;
	float F0 = g_f0 * g_f0;
	F = lerp(F0, albedo, metallic);	
	if (g_fresnelType == FRESNEL_NONE){
		F = F0;
	} 
	if (g_fresnelType == FRESNEL_SCHLICK){
		F = Specular_F_Schlick(F0, NoH);
	}
	if (g_fresnelType == FRESNEL_CT){
		F = Specular_F_CT(F0, NoH);
	}
	F = saturate(F);
	
	const float kS = F;
	const float3 kD = (1.0f - kS) * (1.0f - metallic);

	const float BRDF = (D * F * G) / (4*NoL*NoV);
	const float3 specularColor = saturate(albedo * BRDF);
	const float3 diffuseColor = saturate(albedo * Diffuse_Disney(NoV, NoL, LoH, roughness));

	// return float4(BRDF * kS, BRDF * kS, BRDF * kS, 1.0f);
	return float4(specularColor * kS + diffuseColor * kD, 1);	
	//return float4(roughness, roughness, roughness, 1);
	//return float4(BRDF, BRDF, BRDF, 1);
	//return float4(NoV, NoV, NoV, 1.0f);

	// return float4(D, D, D, 1);
	// return float4(G, G, G, 1);
	// return float4(F, F, F, 1);
}