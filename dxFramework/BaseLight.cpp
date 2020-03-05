#include "BaseLight.h"

BaseLight::BaseLight(BaseLightStruct lightVal)
{
	lightStruct = lightVal;
}

void BaseLight::UpdateLight(BaseLightStruct lightVal)
{
	lightStruct = lightVal;
}
