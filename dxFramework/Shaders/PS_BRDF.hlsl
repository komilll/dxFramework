#include <PS_Input.hlsl> //PixelInputType
#include <ALL_SettingsBRDF.hlsl>
#include <CBuffer_BindingsBRDF.hlsl>

SamplerState baseSampler : register(s0);

Texture2D albedoTexture 	: register(t0);
Texture2D roughnessTexture  : register(t1);
Texture2D normalTexture 	: register(t2);
Texture2D metallicTexture 	: register(t3);

#define PI 3.14159265f

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

float Specular_G_GGX(float roughness, float NoV)
{
	const float a2 = roughness * roughness;
	const float NoV2 = NoV * NoV;
	return (2*NoV) / (NoV + sqrt(a2+(1-a2)*NoV2));
}

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
	const float roughness = g_hasRoughness == 0 ? g_roughnessValue : roughnessTexture.Sample(baseSampler, input.uv);
	const float metallic = g_hasMetallic == 0 ? g_metallicValue : metallicTexture.Sample(baseSampler, input.uv);

	float3 N;
	if (g_hasNormal > 0){
		const matrix<float, 3, 3> TBN = { normalize(input.tangent), normalize(input.binormal), normalize(input.normal) };	
		N = normalTexture.Sample(baseSampler, input.uv);
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

	// float3 albedo = albedoTexture.Sample(baseSampler, input.uv);
	// albedo = albedo / (albedo + float3(1.0, 1.0, 1.0));
	// albedo = pow(albedo, float3(1.0/2.2, 1.0/2.2, 1.0/2.2)); 
		
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

	float G = 1.0f;
	if (g_geometryType == GEOM_GGX){
		G = Specular_G_GGX(roughness, NoV);
	}

	float F = 1.0f;
	if (g_fresnelType == FRESNEL_NONE){
		F = g_f0;
	} 
	if (g_fresnelType == FRESNEL_SCHLICK){
		F = Specular_F_Schlick(g_f0, NoH);
	}
	if (g_fresnelType == FRESNEL_CT){
		F = Specular_F_CT(g_f0, NoH);
	}
	
	const float BRDF = (D * F * G) / (4*NoL*NoV);

	return float4(BRDF, BRDF, BRDF, 1);	
}

