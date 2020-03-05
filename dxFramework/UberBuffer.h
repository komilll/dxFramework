#pragma once
#ifndef _UBER_BUFFER_H_
#define _UBER_BUFFER_H_

#include "framework.h"
using namespace DirectX;

typedef struct _uberBufferStruct {
	XMFLOAT3 viewerPosition;
	float padding;
} UberBufferStruct;
static_assert((sizeof(UberBufferStruct) % 16) == 0, "UberBuffer size must be 16-byte aligned");

#endif // !_UBER_BUFFER_H_
