#include <PS_Input.hlsl> //PixelInputType
#include <ALL_UberBuffer.hlsl> //UberBuffer

struct VertexInputType
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv: TEXCOORD0;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	
	output.position.x = input.position.x;
	output.position.y = input.position.y;
	output.position.z = input.position.z;
	output.position.w = 1.0f;		
	
	output.normal = input.normal;
	output.uv = input.uv;
	output.viewRay.xyz = output.position.xyz - g_viewerPosition;
	output.viewDir.xyz = float3(0,0,0);
	
	return output;
}