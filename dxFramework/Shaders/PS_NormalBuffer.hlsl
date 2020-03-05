#include <PS_Input.hlsl> //PixelInputType

float4 main(PixelInputType input) : SV_TARGET
{
	return float4(input.normal.x, input.normal.y, input.normal.z, 1.0f);
}