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

//////////////
// CLASSES //
//////////////
float3 GetNormalValue(float3 normalMap, float3 tangent, float3 binormal, float3 normal);
float GetRoughness();
float GetMetalness();
float3 GetNormal();
float4 GetBaseColor();

float GeometrySchlickGGX(float NdotV, float k);
float GeometrySmithGGX(float roughness, float NdotV, float NdotL);
float NormalDistributionFunction(float roughness, float NdotH);
float FresnelSchlick(float3 F0, float3 albedo, float metalness, float NdotV, float exp);
float3 FresnelSchlickRoughness(float NdotV, float3 F0, float roughness);

float3 GetPrefilteredColor(float roughness, float3 R);

//////////////////
// CONST VALUES //
//////////////////
static const float PI = 3.14159265359;
//Point light const values
static const float POINT_LIGHT_CONSTANT = 1.0f;
static const float POINT_LIGHT_LINEAR = 0.7f;
static const float POINT_LIGHT_QUADRATIC = 1.8f;
float3 twotothree (float2 x, float y);
float4 twotofour(float2 x, float y, float z);
float4 twotofourtypetwo(float2 x, float2 y);
float add1(float a, float b);
float2 add2(float2 a, float2 b);
float3 add3(float3 a, float3 b);
float4 add4(float4 a, float4 b);
float2 convertfloat2(float x, float y);
float3 convertfloat3(float x, float y, float z);
float4 convertfloat4(float x, float y, float z, float w);
float divide(float a, float b);
float fresnel(float4 F0, float4 albedo, float metalness, float NdotV, float exp);
float get_a(float4 a);
float get_x(float4 a);
float lerp1(float a, float b, float alpha);
float2 lerp2(float2 a, float2 b, float alpha);
float3 lerp3(float3 a, float3 b, float alpha);
float4 lerp4(float4 a, float4 b, float alpha);
float multiply1(float a, float b);
float2 multiply2(float2 a, float2 b);
float3 multiply3(float3 a, float3 b);
float4 multiply4(float4 a, float4 b);
float4 sampletexture(float4 t, float2 uv);
float substract(float a, float b);

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 ColorPixelShader(PixelInputType input) : SV_TARGET
{
//GAMMA CORRECTION
	albedo = GetBaseColor();
//SPECULAR IBL	
	float3 WorldPos = input.position.xyz;

	float3 N = normalize(Normal);
    float3 V = normalize(input.viewDirection);
	float3 R = reflect(-input.viewDirection, input.normal);

	float NdotV = max(dot(N, V), 0.0);

//FRESNEL EQUATION
	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, albedo, metalness);
	float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
//////////////////
//DIFFUSE IRRADIANCE
	float3 kS = F;
	float3 kD = 1.0 - kS;
	kD *= (1.0 - metalness);

//AMBIENT CALCULATIONS
	float3 ambient = (kD * diffuse);// * ao;

//MULTIPLE DIRECTIONAL LIGHT PASS
	float4 finalColor = float4(ambient, 1.0f); //Base of output color is ambient lighting
	float4 directionalLight = 0;
	
#if NUM_LIGHTS_DIRECTIONAL > 0
	for (int i = 0; i < NUM_LIGHTS_DIRECTIONAL; i++)
		accumulatedLightStrength += g_directionalLightDirectionStrength[i].w;

	for (int i = 0; i < NUM_LIGHTS_DIRECTIONAL; i++) //Iterate over number of directional lights
	{	
		float3 L = normalize(g_directionalLightDirectionStrength[i].xyz);
		float3 H = normalize(V + L);
		float3 radiance = g_directionalLightColor[i].xyz * g_directionalLightDirectionStrength[i].w;
		
		// //Cook-torrance brdf
		float NdotH = max(dot(N, H), 0.0f);
		float NdotV = max(dot(N, V), 0.0f);
		float NdotL = max(dot(N, L), 0.0f);
		
		float NDF = NormalDistributionFunction(roughness, NdotH);
		float G = GeometrySmithGGX(roughness, NdotV, NdotL);		
		
		float3 numerator = NDF * G * F;
		float denominator = 4.0 * max(NdotV, 0.0) * max(NdotL, 0.0);
		specular = numerator / max(denominator, 0.001);

		finalColor += float4(((kD * albedo / PI) + specular) * radiance * NdotL, 1.0f);
	}
#endif

//MULTIPLE POINT LIGHT PASS
	accumulatedLightStrength = 0.0f;
	float4 pointLight;
#if NUM_LIGHTS_POINT > 0
	for (int i = 0; i < NUM_LIGHTS_POINT; i++)
		accumulatedLightStrength += g_pointLightColorWithStrength[i].w;

	for (int i = 0; i < NUM_LIGHTS_POINT; i++) //Iterate over number of point lights
	{
		float3 L = normalize(input.worldPosition.xyz - g_pointLightPositionWithRadius[i].xyz);
		float3 H = normalize(V - L);
		float distance = length(input.worldPosition.xyz - g_pointLightPositionWithRadius[i].xyz);
		float attenuation = 1.0f / (distance * distance);
		float3 radiance = g_pointLightColorWithStrength[i].xyz * attenuation * (distance / g_pointLightPositionWithRadius[i].w);
		
		// //Cook-torrance brdf
		float NdotH = max(dot(N, H), 0.0f);
		float NdotV = max(dot(N, V), 0.0f);
		float NdotL = max(dot(N, -L), 0.0f);
		
		float NDF = NormalDistributionFunction(roughness, NdotH);
		float G = GeometrySmithGGX(roughness, NdotV, NdotL);		
		
		float3 numerator = NDF * G * F;
		float denominator = 4.0 * max(NdotV, 0.0) * max(NdotL, 0.0);
		specular = numerator / max(denominator, 0.001);

		//finalColor += float4(((kD * albedo / PI) + specular) * radiance * NdotL, 1.0f);
	}
	//finalColor += normalize(pointLight);
#endif
	return finalColor * g_albedoTint;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}

float NormalDistributionFunction(float roughness, float NdotH)
{
	float a2 = max(roughness * roughness, 0.001);
	float denom = 1.0 + NdotH * NdotH * (a2 - 1.0);
	denom = denom * denom;
	denom = denom * PI;
	
	return a2 / denom;
}

float GeometrySmithGGX(float roughness, float NdotV, float NdotL)
{
	float k = (roughness + 1);
	k = (k * k) / 8;
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);

	return ggx1 * ggx2;
}

float FresnelSchlick(float3 F0, float3 albedo, float metalness, float NdotV, float exp)
{
	F0 = lerp(F0, albedo, metalness);

	return F0 + (1.0 - F0) * pow((1.0 - NdotV), exp);
}

float3 FresnelSchlickRoughness(float NdotV, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);
}   

////////////////////////////////////////////////////////////////////////////////
// GET TEXTURE PBR VALUES
////////////////////////////////////////////////////////////////////////////////
float3 GetNormalValue(float3 normalMap, float3 tangent, float3 binormal, float3 normal)
{
	normalMap = (normalMap * 2.0f) - 1.0f; //Convert from [0, 1] to [-1, 1]
	normal = (normalMap.x * tangent) + (normalMap.y * binormal) + (normalMap.z * normal);
	return normalize(normal);
}

////////////////////////////////////////////////////////////
// OTHER CALCULATIONS
////////////////////////////////////////////////////////////
float3 GetPrefilteredColor(float roughness, float3 R)
{
	float3 color_0 = environmentMap_1.Sample(SampleType, R);
	float3 color_1 = environmentMap_2.Sample(SampleType, R);
	float3 color_2 = environmentMap_3.Sample(SampleType, R);
	float3 color_3 = environmentMap_4.Sample(SampleType, R);
	float3 color_4 = environmentMap_5.Sample(SampleType, R);

	float3 colors[5] = {color_0, color_1, color_2, color_3, color_4};
	int index = clamp(roughness * 5, 0, 4);
	float lerpVal = (roughness - index * 0.2f) / ((index + 1) * 0.2f - index * 0.2f);
	lerpVal = clamp(lerpVal, 0.0f, 1.0f);

	return lerp(colors[index], colors[clamp(index + 1, 0, 4)], lerpVal);
}

////////////////////////////////////////////////////////////////////////////////
// SHADER EDITOR METHODS
////////////////////////////////////////////////////////////////////////////////
float3 twotothree (float2 x, float y)
{
	return float3(x, y);
}
float4 twotofour(float2 x, float y, float z)
{
	return float4(x, y, z);
}
float4 twotofourtypetwo(float2 x, float2 y)
{
	return float4(x, y);
}
float add1(float a, float b)
{
	return a + b;
}
float2 add2(float2 a, float2 b)
{
	return a + b;
}
float3 add3(float3 a, float3 b)
{
	return a + b;
}
float4 add4(float4 a, float4 b)
{
	return a + b;
}
float2 convertfloat2(float x, float y)
{
	return float2(x, y);
}
float3 convertfloat3(float x, float y, float z)
{
	return float3(x, y, z);
}
float4 convertfloat4(float x, float y, float z, float w)
{
	return float4(x, y, z, w);
}
float divide(float a, float b)
{
	return a / b;
}




float fresnel(float4 F0, float4 albedo, float metalness, float NdotV, float exp)
{
	F0 = lerp(F0, albedo, metalness);

	return F0 + (1.0 - F0) * pow((1.0 - NdotV), exp);
}
float get_a(float4 a)
{
	return a.w;
}
float get_x(float4 a)
{
	return a.r;
}
float lerp1(float a, float b, float alpha)
{
	alpha = saturate(alpha);
	return lerp(a, b, alpha);
}
float2 lerp2(float2 a, float2 b, float alpha)
{
	alpha = saturate(alpha);
	return lerp(a, b, alpha);
}
float3 lerp3(float3 a, float3 b, float alpha)
{
	alpha = saturate(alpha);
	return lerp(a, b, alpha);
}
float4 lerp4(float4 a, float4 b, float alpha)
{
	alpha = saturate(alpha);
	return lerp(a, b, alpha);
}
float multiply1(float a, float b)
{
	return a * b;
}
float2 multiply2(float2 a, float2 b)
{
	return a * b;
}
float3 multiply3(float3 a, float3 b)
{
	return a * b;
}
float4 multiply4(float4 a, float4 b)
{
	return a * b;
}
float4 sampletexture(float4 t, float2 uv)
{
	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}
float substract(float a, float b)
{
	return a - b;
}


////////////////////////////////////////////////////////////////////////////////
// SHADER EDITOR METHODS
////////////////////////////////////////////////////////////////////////////////
float4 GetBase Color()
{
	return tex_0;
}
float GetMetalness()
{
	float B = 0.0f;

	return B;
}
float GetRoughness()
{
	float K = 1.0f;

	return K;
}
float3 GetNormal()
{
	return float3(1.0f, 1.0f, 1.0f);
}
float3 GetEmissive Color()
{
	float3 L = float3(1.0f, 0.0f, 0.0f);

	return L;
}
