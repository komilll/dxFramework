#pragma once
#ifndef _PROFILER_H_
#define _PROFILER_H_

#include "framework.h"
#include <map>
#include <ctime>
#include <chrono>

class Profiler 
{
public:
	struct ClockData {
		double totalTime;
		double dataCount;
		ID3D11Query* tsStart;
		ID3D11Query* tsEnd;
	};

	struct FrameData {
		std::map<std::string, ClockData> data;
		ID3D11Query* disjoint;
	};

	Profiler(ID3D11Device* device, ID3D11DeviceContext* context);

	void StartFrame();
	void StartProfiling(std::string profileKey);
	void EndProfiling(std::string profileKey);
	void ResetProfiling();
	void EndFrame();

	FrameData* CopyProfilingDataNewFrame() { return &m_frames[m_activeBuffer]; };
	FrameData* CopyProfilingDataDoneFrame() { return &m_frames[GetBufferToCheckIndex()]; };

private:
	void CollectProfilingData();
	int GetBufferToCheckIndex() const { /*return m_activeBuffer & 0;*/ return 0; };

private:
	FrameData m_frames[2];
	int m_activeBuffer = 0;
	bool m_isFrameReady = true;
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
};

#endif // !_PROFILER_H_
