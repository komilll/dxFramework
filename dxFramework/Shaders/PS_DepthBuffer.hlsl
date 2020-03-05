#include <PS_Input.hlsl> //PixelInputType

float4 main(PixelInputType input) : SV_TARGET
{
	const float depth = input.position.z / input.position.w;
	return float4(depth, depth, depth, 1.0f);
}