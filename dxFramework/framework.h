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
#include <time.h>
#include <stdio.h>

namespace Framework
{
	static float lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}

	//Copied from here - https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c
	static std::string currentDateTime() {
		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		errno_t err = localtime_s(&tstruct, &now);
		if (err == 0)
		{
			// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
			// for more information about date/time format
			strftime(buf, sizeof(buf), "%d_%m_%Y__%H_%M_%S", &tstruct);

			return buf;
		}
		return "UKNOWN_TIME";
	}
};