#include <PS_Input.hlsl> //PixelInputType

float4 main(PixelInputType input) : SV_TARGET
{
	return float4(input.position.x, input.position.y, input.position.z, 1.0f);
}