#pragma once
#ifndef _BaseLight_H_
#define _BaseLight_H_

#include "framework.h"

using namespace DirectX;

class BaseLight
{
public:
	typedef struct _baseLightStruct {
		XMFLOAT4 positionOrDirection; //w == 0 for position and w == 1 for direction
		float intensity;		
	} BaseLightStruct;

	BaseLight() = default;
	BaseLight(BaseLightStruct lightVal);
	void UpdateLight(BaseLightStruct lightVal);
	BaseLightStruct GetLightStruct() const { return lightStruct; };

protected:
	BaseLightStruct lightStruct;
};

#endif // !_BaseLight_H_