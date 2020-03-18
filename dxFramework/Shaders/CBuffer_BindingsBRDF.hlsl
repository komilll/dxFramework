#ifndef _BRDF_BINDINGS_HLSL_
#define _BRDF_BINDINGS_HLSL_

cbuffer SpecialBufferBRDF : register(b13)
{
	int g_ndfType;
	int g_geometryType;
	int g_fresnelType;

	int g_hasAlbedo;
	int g_hasNormal;
	int g_hasRoughness;
	int g_hasMetallic;
	
	float g_roughnessValue;
	float g_metallicValue;
	float g_f0;

	int g_debugType;
	
	float g_paddingSpecialBufferBRDF;
};

#endif //_BRDF_BINDINGS_HLSL_