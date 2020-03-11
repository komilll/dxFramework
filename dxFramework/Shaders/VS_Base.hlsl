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
	
	output.pointToLight.xyz = g_directionalLightDirection;
	output.pointToLight.w = g_directionalLightIntensity;
	
	output.position = mul(output.position, worldMatrix);	
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projectionMatrix);
	
	output.normal =		mul(input.normal, worldMatrix);
	output.tangent = 	mul(input.tangent, worldMatrix);
	output.binormal = 	mul(input.binormal, worldMatrix);

	output.viewRay.xyz = output.position.xyz - g_viewerPosition;
	output.viewDir.xyz = output.position.xyz - g_directionalLightDirection;
	
	output.uv = input.uv;
	
	return output;
}