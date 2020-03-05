#pragma once

#include "targetver.h"

#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "DirectXHelpers.h"
#include "GeometricPrimitive.h"
#include "GraphicsMemory.h"
#include "Model.h"
#include "PrimitiveBatch.h"
#include "SimpleMath.h"
#include "VertexTypes.h"

#pragma comment(lib,"d3d11.lib")
#include <d3d11.h>
#define WIN32_LEAN_AND_MEAN             

#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>
#include <Windows.h>
#include <tchar.h>
#include <memory>
#include <memory.h>

namespace Framework
{
	static float lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}
};