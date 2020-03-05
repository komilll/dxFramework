#pragma once

#include "BaseLight.h"

class DirectionalLight : public BaseLight
{
public:
	DirectionalLight() = default;
	DirectionalLight(BaseLightStruct lightVal);
};