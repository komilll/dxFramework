#include <PS_Input.hlsl> //PixelInputType
#include <ALL_UberBuffer.hlsl> //UberBuffer

cbuffer MatrixBuffer : register(b0)
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
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
	
	output.position.x = input.position.x;
	output.position.y = input.position.y;
	output.position.z = input.position.z;
	output.position.w = 1.0f;		
	
	const float4 worldPos = mul(output.position, worldMatrix);
	
	output.normal = input.normal;
	output.uv = input.uv;
	output.viewRay.xyz = worldPos.xyz - g_viewerPosition;	
	output.viewDir.xyz = float3(0,0,0);
	
	return output;
}