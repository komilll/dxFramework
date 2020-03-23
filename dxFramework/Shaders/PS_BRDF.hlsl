#include <PS_Input.hlsl> //PixelInputType
#include <ALL_SettingsBRDF.hlsl>
#include <CBuffer_BindingsBRDF.hlsl>
#include <PS_IBL.hlsl>

Texture2D albedoTexture 	: register(t0);
Texture2D roughnessTexture  : register(t1);
Texture2D normalTexture 	: register(t2);
Texture2D metallicTexture 	: register(t3);

#define PI 3.14159265f
#define invPI 1.0f / 3.14159265f

//Normal distribution functions
float Specular_D_Beckmann(float roughness, float NoH)
{
    const float a = roughness * roughness;
	const float a2 = a * a;
    const float NoH2 = NoH * NoH;
    return exp( (NoH2-1.0f) / (a2*NoH2) ) / (a2*PI*NoH2*NoH2);
}

float Specular_D_GGX(float roughness, float NoH)
{
    const float a = roughness * roughness;
	const float a2 = a * a;
    const float NoH2 = NoH * NoH;
    return a2 / (PI * pow((NoH2*(a2-1)+1), 2) );
}

float Specular_D_Blinn_Phong(float roughness, float NoH)
{
	const float a = roughness * roughness;
    const float a2 = a * a;
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
	const float a = roughness * roughness;
	const float c = NoV / (a * sqrt(1 - NoV*NoV));
	if (c >= 1.6f)
		return 1.0f;
	else
		return (3.535f*c + 2.181f*c*c) / (1.0f + 2.276f*c + 2.577f*c*c);
}

float Specular_G_GGX(float roughness, float NoV)
{
	const float a = roughness * roughness;
	const float a2 = a * a;
	const float NoV2 = NoV * NoV;
	return (2.0f*NoV) / (NoV + sqrt(a2+(1-a2)*NoV2));
}

float Specular_G_SchlickBeckmann(float roughness, float NoV)
{
	const float a = roughness * roughness;
	const float k = a * sqrt(2.0f/PI);
	return NoV / (NoV*(1-k) + k);
}

float Specular_G_SchlickGGX(float roughness, float NoV)
{
	const float a = roughness * roughness;
	const float k = a/2.0f;
	return NoV / (NoV*(1.0f-k) + k);
}

//Fresnel functions
float Specular_F_Schlick(float f0, float VoH)
{
	return f0 + (1.0f-f0)*pow(1-VoH, 5.0f);
}
float Specular_F_Schlick(float f0, float f90, float VoH)
{
	return f0 + (f90-f0)*pow(1.0f-VoH, 5.0f);
}

float Specular_F_CT(float f0, float VoH)
{
	const float f0Sqrt = sqrt(f0);
	const float eta = (1.0f+f0Sqrt)/(1.0f-f0Sqrt);
	const float g = sqrt(eta*eta + VoH*VoH - 1.0f);
	const float c = VoH;

	return 0.5f * pow((g-c)/(g+c), 2) * pow((1 + ((g+c)*c-1) / ((g-c)*c + 1)), 2);
}

//Disney diffuse function - modified by Frostbite
//https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf#page=11&zoom=auto,-265,590
float Diffuse_Disney(float NoV, float NoL, float LoH, float roughness)
{
	float  energyBias     = lerp(0, 0.5,  roughness);
	float  energyFactor   = lerp (1.0, 1.0 / 1.51,  roughness);
	float  fd90           = energyBias + 2.0 * LoH*LoH * roughness;
    float f0              = 1.0f;
    float lightScatter    = Specular_F_Schlick(f0, fd90, NoL);
    float viewScatter     = Specular_F_Schlick(f0, fd90, NoV);
	return  lightScatter * viewScatter * energyFactor;

	// float fd90 = 0.5f + 2 * roughness * LoH * LoH;
	// return (1.0f+(fd90-1.0f)*pow(1.0f-NoL, 5)) * (1.0f+(fd90-1.0f)*pow(1.0f-NoV, 5));
}

float4 main(PixelInputType input) : SV_TARGET
{	
	float roughness = g_hasRoughness == 0 ? g_roughnessValue : roughnessTexture.Sample(baseSampler, input.uv).r;
	float metallic = g_hasMetallic == 0 ? g_metallicValue : metallicTexture.Sample(baseSampler, input.uv).r;

	roughness = clamp(roughness, 0.0001f, 0.999f);
	// roughness = pow(roughness, 2.0f);

	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	input.binormal = normalize(input.binormal);

	float3 N;
	if (g_hasNormal > 0){
		const matrix<float, 3, 3> TBN = { input.tangent, input.binormal, input.normal };
		N = normalTexture.Sample(baseSampler, input.uv).rgb;
		N = normalize(N * 2.0f - 1.0f);
		N = normalize(mul(TBN, N));
	}
	else{
		N = normalize(input.normal);
	}

	const float3 V = normalize(input.viewDir.xyz);
	const float3 L = normalize(input.pointToLight.xyz);
	const float intensity = input.pointToLight.w;	
	const float3 H = normalize(L + V);
	
	const float NoV = abs(dot(N, V)) + 0.0001f; //avoid artifact - as in Frostbite
	const float NoH = saturate(dot(N, H));
	const float LoH = saturate(dot(L, H));
	const float LoV = saturate(dot(L, V));
	const float VoH = saturate(dot(V, H));
	const float NoL = saturate(dot(L, N));

	float3 albedo = g_hasAlbedo == 0 ? float3(1.0f, 0.0f, 0.0f) : albedoTexture.Sample(baseSampler, input.uv);
	albedo = saturate(albedo);
	// albedo = albedo / (albedo + float3(1.0, 1.0, 1.0));
	// albedo = pow(albedo, float3(1.0/2.2, 1.0/2.2, 1.0/2.2)); 

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
	float G1 = 1.0f; //Debug only purposes
	float G2 = 1.0f; //Debug only purposes
	if (g_geometryType == GEOM_IMPLICIT){
		G = G1 = Specular_G_Implicit(NoL, NoV);
	}
	if (g_geometryType == GEOM_NEUMANN){
		G = G1 = Specular_G_Neumann(NoL, NoV);
	}
	if (g_geometryType == GEOM_COOK_TORRANCE){
		G = G1 = Specular_G_CT(NoH, NoV, NoL, VoH);
	}
	if (g_geometryType == GEOM_KELEMEN){
		G = G1 = Specular_G_Kelemen(NoV, NoL, VoH);
	}
	if (g_geometryType == GEOM_BECKMANN){
		G1 = Specular_G_Beckmann(NoV, roughness);
		G2 = Specular_G_Beckmann(NoL, roughness);
		G = G1 * G2;
	}
	if (g_geometryType == GEOM_GGX){
		G1 = Specular_G_GGX(roughness, NoV);
		G2 = Specular_G_GGX(roughness, NoL);
		G = G1 * G2;
	}
	if (g_geometryType == GEOM_SCHLICK_BECKMANN){
		G1 = Specular_G_SchlickBeckmann(roughness, NoV);
		G2 = Specular_G_SchlickBeckmann(roughness, NoL);
		G = G1 * G2;
	}
	if (g_geometryType == GEOM_SCHLICK_GGX){
		G1 = Specular_G_SchlickGGX(roughness, NoV);
		G2 = Specular_G_SchlickGGX(roughness, NoL);
		G = G1 * G2;
	}
	G = saturate(G);

	//F component
	float F = 0;
	float ior = 2.5f; //Steel
	float F0 = abs((1.0f - ior) / (1.0f + ior));
	F0 = F0 * F0;
	F = lerp(F0, albedo, metallic);	
	if (g_fresnelType == FRESNEL_NONE){
		F = F0;
	} 
	if (g_fresnelType == FRESNEL_SCHLICK){
		F = Specular_F_Schlick(F0, VoH);
	}
	if (g_fresnelType == FRESNEL_CT){
		F = Specular_F_CT(F0, VoH);
	}
	F = saturate(F);
	
	const float3 diffuseColor = albedo - albedo * metallic;
	const float3 specularColor = lerp(0.04f, albedo, metallic);
	const float kS = F;
	const float kD = (1.0f - kS) * (1.0f - metallic);

	const float numeratorBRDF = D * F * G;
	const float denominatorBRDF = max((4.0f * max(NoV, 0.0f) * max(NoL, 0.0f)), 0.001f);
	const float BRDF = numeratorBRDF / denominatorBRDF;

	const float3 spec = saturate(albedo * BRDF) * specularColor;

	const float ambient = 0.05f;
	// const float3 diff = saturate(NoL) * diffuseColor;

	const float3 diff = saturate(invPI * Diffuse_Disney(NoV, NoL, LoH, roughness)) * diffuseColor;

	if (g_debugType == DEBUG_NONE){
		const float3 specularIBL = SpecularIBL(roughness, N) * specularColor;// * BRDF * NoL;
		return float4(specularIBL, 1.0f);
		return float4(diff + spec, 1.0f);
	}
	if (g_debugType == DEBUG_DIFF){
		return float4(diff, 1.0f);
	}
	if (g_debugType == DEBUG_SPEC){
		return float4(spec, 1.0f);
	}
	if (g_debugType == DEBUG_ALBEDO){
		return float4(albedo, 1.0f);
	}
	if (g_debugType == DEBUG_NORMAL){
		return float4(N, 1.0f);
	}
	if (g_debugType == DEBUG_ROUGHNESS){
		return float4(roughness, roughness, roughness, 1.0f);
	}
	if (g_debugType == DEBUG_METALLIC){
		return float4(metallic, metallic, metallic, 1.0f);
	}
	if (g_debugType == DEBUG_NDF){
		return float4(D, D, D, 1.0f);
	}
	if (g_debugType == DEBUG_GEOM){
		return float4(G1, G2, 0.0f, 1.0f);
	} 
	if (g_debugType == DEBUG_FRESNEL){
		return float4(F, F, F, 1.0f);
	}

	return float4(1.0f, 0.0f, 1.0f, 1.0f);
}