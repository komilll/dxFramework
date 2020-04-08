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
		LightType type;
		XMFLOAT3 position;
		XMFLOAT4 color;
	} BaseLightStruct;

	BaseLight() = default;
	BaseLight(BaseLightStruct lightVal);
	void UpdateLight(BaseLightStruct lightVal);
	BaseLightStruct GetLightStruct() const { return lightStruct; };

protected:
	BaseLightStruct lightStruct;
};

#endif // !_BaseLight_H_