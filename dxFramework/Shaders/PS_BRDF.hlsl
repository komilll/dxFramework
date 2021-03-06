#ifndef _PS_BRDF_HLSL_
#define _PS_BRDF_HLSL_

#include <PS_Input.hlsl> //PixelInputType
#include <ALL_SettingsBRDF.hlsl>
#include <CBuffer_BindingsBRDF.hlsl>
#include <PS_PrecomputeIBL.hlsl>
#include <PS_BRDF_Helper.hlsl>
#include "LightSettings.hlsl"
#include "AreaLights.hlsl"

SamplerState BrdfLutSampleType : register(s1)
{
    Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    MipLODBias = 0.0f;
    MaxAnisotropy = 1;
    ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    BorderColor[0] = 0;
    BorderColor[1] = 0;
    BorderColor[2] = 0;
    BorderColor[3] = 0;
    MinLOD = 0;
    MaxLOD = D3D11_FLOAT32_MAX;
};

Texture2D albedoTexture 	: register(t0);
Texture2D roughnessTexture  : register(t1);
Texture2D normalTexture 	: register(t2);
Texture2D metallicTexture 	: register(t3);

StructuredBuffer<Light> lightSettings : register(t13);

////Normal distribution functions
//float Specular_D_Beckmann(float roughness, float NoH)
//{
//    const float a = roughness * roughness;
//	const float a2 = a * a;
//    const float NoH2 = NoH * NoH;
//    return exp( (NoH2-1.0f) / (a2*NoH2) ) / (a2*PI*NoH2*NoH2);
//}

//float Specular_D_GGX(float roughness, float NoH)
//{
//    const float a = roughness * roughness;
//	const float a2 = a * a;
//    const float NoH2 = NoH * NoH;
//    return a2 / (PI * pow((NoH2*(a2-1)+1), 2) );
//}

//float Specular_D_Blinn_Phong(float roughness, float NoH)
//{
//	const float a = roughness * roughness;
//    const float a2 = a * a;
//    const float NoH2 = NoH * NoH;
//    return pow(NoH, (2/a2-2)) / (PI*a2);
//}

////Geometry shadowing functions
//float Specular_G_Implicit(float NoL, float NoV)
//{
//	return NoL * NoV;
//}

//float Specular_G_Neumann(float NoL, float NoV)
//{
//	return (NoL * NoV) / max(NoL, NoV);
//}

//float Specular_G_CT(float NoH, float NoV, float NoL, float VoH)
//{
//	return min( min(1.0f, (2.0f*NoH*NoV/VoH)), (2.0f*NoH*NoL/VoH) );
//}

//float Specular_G_Kelemen(float NoV, float NoL, float VoH)
//{
//	return NoL*NoV/(VoH*VoH);
//}

//float Specular_G_Beckmann(float NoV, float roughness)
//{
//	const float a = roughness * roughness;
//	const float c = NoV / (a * sqrt(1 - NoV*NoV));
//	if (c >= 1.6f)
//		return 1.0f;
//	else
//		return (3.535f*c + 2.181f*c*c) / (1.0f + 2.276f*c + 2.577f*c*c);
//}

//float Specular_G_GGX(float roughness, float NoV)
//{
//	const float a = roughness * roughness;
//	const float a2 = a * a;
//	const float NoV2 = NoV * NoV;
//	return (2.0f*NoV) / (NoV + sqrt(a2+(1-a2)*NoV2));
//}

//float Specular_G_SchlickBeckmann(float roughness, float NoV)
//{
//	const float a = roughness * roughness;
//	const float k = a * sqrt(2.0f/PI);
//	return NoV / (NoV*(1-k) + k);
//}

//float Specular_G_SchlickGGX(float roughness, float NoV)
//{
//	const float a = roughness * roughness;
//	const float k = a/2.0f;
//	return NoV / (NoV*(1.0f-k) + k);
//}

////Fresnel functions
//float3 Specular_F_Schlick(float3 f0, float VoH)
//{
//	return f0 + (1.0-f0)*pow(1.0-VoH, 5.0);
//}
//float3 Specular_F_Schlick(float3 f0, float3 f90, float VoH)
//{
//	return f0 + (f90-f0)*pow(1.0-VoH, 5.0);
//}

//float3 Specular_F_CT(float3 f0, float VoH)
//{
//    const float3 f0Sqrt = min(0.999, sqrt(f0));
//	const float3 eta = (1.0 + f0Sqrt) / (1.0 - f0Sqrt);
//	const float3 g = sqrt(eta*eta + VoH*VoH - 1.0);
//	const float3 c = VoH;
    
//    float3 g_minus = g - VoH;
//    float3 g_plus = g + VoH;
//    float3 A = (g_minus / g_plus);
//    float3 B = (g_plus * VoH - 1.0) / (g_minus * VoH + 1.0);

//    return 0.5 * A * A * (1.0 + B * B);
//}

////Disney diffuse function - modified by Frostbite
////https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf#page=11&zoom=auto,-265,590
//float Diffuse_Disney(float NoV, float NoL, float LoH, float roughness)
//{
//	float  energyBias     = lerp(0, 0.5,  roughness);
//	float  energyFactor   = lerp (1.0, 1.0 / 1.51,  roughness);
//	float3  fd90          = energyBias + 2.0 * LoH*LoH * roughness;
//    float3 f0             = 1.0f;
//    float lightScatter    = Specular_F_Schlick(f0, fd90, NoL);
//    float viewScatter     = Specular_F_Schlick(f0, fd90, NoV);
//	return  lightScatter * viewScatter * energyFactor;

//	// float fd90 = 0.5f + 2 * roughness * LoH * LoH;
//	// return (1.0f+(fd90-1.0f)*pow(1.0f-NoL, 5)) * (1.0f+(fd90-1.0f)*pow(1.0f-NoV, 5));
//}

float4 main(PixelInputType input) : SV_TARGET
{	
	float roughness = g_hasRoughness == 0 ? g_roughnessValue : roughnessTexture.Sample(baseSampler, input.uv).r;
	float metallic = g_hasMetallic == 0 ? g_metallicValue : metallicTexture.Sample(baseSampler, input.uv).r;

	roughness = clamp(roughness, 0.05f, 0.999f);
	//roughness = pow(roughness, 2.0f);

	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	input.binormal = normalize(input.binormal);

	float3 N = 0;
	if (g_hasNormal > 0){
		float3x3 TBN = transpose(float3x3(input.tangent, input.binormal, input.normal));
		N = normalTexture.Sample(baseSampler, input.uv).rgb * 2.0 - 1.0;
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

	const float3 R = 2.0 * NoV * N - V;

	float3 albedo = g_hasAlbedo == 0 ? float3(1.0f, 0.0f, 0.0f) : albedoTexture.Sample(baseSampler, input.uv);
    albedo = saturate(albedo);
    albedo = 1;
	// albedo = albedo / (albedo + float3(1.0, 1.0, 1.0));
    //albedo = pow(albedo, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

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

    const float3 diffuseColor = saturate(albedo - albedo * metallic);
    const float3 specularColor = saturate(lerp(0.04f, albedo, metallic));
	//F component
	float3 F = 0;
	if (g_fresnelType == FRESNEL_NONE){
        F = specularColor;
    } 
	if (g_fresnelType == FRESNEL_SCHLICK){
        F = Specular_F_Schlick(specularColor, VoH);
    }
	if (g_fresnelType == FRESNEL_CT){
        F = Specular_F_CT(specularColor, VoH);
    }
	F = saturate(F);
	
	const float3 prefilteredDiffuse = diffuseIBLTexture.Sample(baseSampler, N).rgb;
	const float3 prefilteredSpecular = specularIBLTexture.SampleLevel(baseSampler, R, roughness * 5.0);

	const float numeratorBRDF = D * F * G;
	//const float denominatorBRDF = max((4.0f * max(NoV, 0.0f) * max(NoL, 0.0f)), 0.001f);
	//const float BRDF = numeratorBRDF / denominatorBRDF;
    const float3 sunLight = numeratorBRDF * NoL * g_directionalLightColor.w;

    float2 envBRDF = enviroBRDF.Sample(BrdfLutSampleType, float2(NoV, roughness)).rg;
    float3 diffuse = prefilteredDiffuse * diffuseColor;
    float3 specular = prefilteredSpecular * (specularColor * envBRDF.x + envBRDF.y);
    
    //return MonteCarloSpecular(input.positionWS.xyz, diffuseColor, specularColor, N, V, roughness, 512);
    uint lightCount;
    uint stride;
    lightSettings.GetDimensions(lightCount, stride);
    
    float4 color = 0;
    const float4 environmentLight = float4(diffuse + sunLight + specular, 1.0f);
    color = environmentLight;
    
    float3 diffAreaLight = DiffuseSphereLight_ViewFactor(input.positionWS.xyz, diffuse, N, lightSettings[0]);
    float3 specAreaLight = SpecularSphereLight_KarisMRP(input.positionWS.xyz, specular, roughness, N, V, lightSettings[0]);
    
    color = float4((diffAreaLight + specAreaLight) * lightSettings[0].color, 1.0f);
    
	if (g_debugType == DEBUG_NONE){
        return color;
        return environmentLight;
    }
	if (g_debugType == DEBUG_DIFF){
        return float4(/*diffuse + */ diffAreaLight * lightSettings[0].color, 1.0);
        return float4(diffuse, 1.0f);
    }
	if (g_debugType == DEBUG_SPEC){
        return float4(/*specular + */ specAreaLight * lightSettings[0].color, 1.0);
        return float4(sunLight + specular, 1.0f);
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
		return float4(F, 1.0f);
	}

	return float4(1.0f, 0.0f, 1.0f, 1.0f);
}

#endif //_PS_BRDF_HLSL_