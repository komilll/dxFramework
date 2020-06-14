struct PixelInputType
{
	float4 position : SV_POSITION;
    float3 positionWS : TEXCOORD0;
};

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
    float3 instanceColor : TEXCOORD1;
    float3 instancePosition : TEXCOORD2;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	
	float4 worldPos = mul(float4(input.position.xyz, 1.0f), worldMatrix);
    output.positionWS = worldPos.xyz;

    output.position = mul(worldPos, viewMatrix);
	output.position = mul(output.position, projectionMatrix).xyww;

	return output;
}