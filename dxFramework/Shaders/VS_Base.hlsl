#include <PS_Input.hlsl> //PixelInputType
#include <ALL_UberBuffer.hlsl> //UberBuffer

cbuffer MatrixBuffer : register(b0)
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

cbuffer DirectionalLightBuffer : register(b1)
{
	float3 g_directionalLightDirection;
	float g_directionalLightIntensity;
};

struct VertexInputType
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv: TEXCOORD0;	
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
		
	output.position = float4(input.position, 1.0f);
	float4 positionWS = mul(output.position, worldMatrix);
	
	output.pointToLight.xyz = normalize(g_directionalLightDirection);
	output.pointToLight.w = g_directionalLightIntensity;
	
	output.position = mul(positionWS, viewMatrix);
	output.position = mul(output.position, projectionMatrix);
	
	output.normal =		normalize( mul(input.normal, (float3x3)worldMatrix) );
	output.tangent = 	normalize( mul(input.tangent, (float3x3)worldMatrix) );
	output.binormal = 	normalize( mul(input.binormal, (float3x3)worldMatrix) );

	output.viewRay.xyz = positionWS.xyz - g_viewerPosition;
	output.viewDir.xyz = normalize(g_viewerPosition.xyz - positionWS.xyz);

	// output.viewRay.xyz = output.position.xyz - g_viewerPosition;
	// output.viewDir.xyz = output.position.xyz - g_directionalLightDirection;
	
	output.uv = input.uv;
	
	return output;
}