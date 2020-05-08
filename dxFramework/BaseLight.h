#pragma once
#ifndef _BaseLight_H_
#define _BaseLight_H_

#include "framework.h"

using namespace DirectX;

class BaseLight
{
public:
	enum class LightType : int
	{
		Point		= 0,
		Directional = 1,
		Area		= 2,
	};

	typedef struct _baseLightStruct {
		XMFLOAT3 position;
		float radius;
		XMFLOAT3 color;
		float area;

		int type; /*LightType*/ 
		XMFLOAT3 padding;
	} BaseLightStruct;

	BaseLight() = default;
	BaseLight(BaseLightStruct lightVal);
	void UpdateLight(BaseLightStruct lightVal);
	BaseLightStruct GetLightStruct() const { return lightStruct; };

protected:
	BaseLightStruct lightStruct;
};

#endif // !_BaseLight_H_