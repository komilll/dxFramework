#pragma once
#ifndef _BaseLight_H_
#define _BaseLight_H_

#include "framework.h"

using namespace DirectX;

class BaseLight
{
public:
	enum class LightType
	{
		None		= 0,
		Point		= 1,
		Directional = 2,
		Area		= 4,
		Environment	= 8
	};

	typedef struct _baseLightStruct {
		int type;
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