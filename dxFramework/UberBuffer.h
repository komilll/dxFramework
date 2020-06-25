#pragma once
#ifndef _UBER_BUFFER_H_
#define _UBER_BUFFER_H_

#include "framework.h"
using namespace DirectX;

typedef struct _uberBufferStruct {
	XMFLOAT4 unlitColor;
	XMFLOAT4 directionalLightColor;
	XMFLOAT3 directionalLightDirection;
	float pad1 = 0;

	XMFLOAT3 viewerPosition;
	float pad2 = 0;

	XMFLOAT3 directionalLightPosition;
	float pad3 = 0;

	XMMATRIX lightViewMatrix;
	XMMATRIX lightProjMatrix;
} UberBufferStruct;
static_assert((sizeof(UberBufferStruct) % 16) == 0, "UberBuffer size must be 16-byte aligned");

#endif // !_UBER_BUFFER_H_
