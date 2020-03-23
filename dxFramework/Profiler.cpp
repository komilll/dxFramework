#include "Profiler.h"


Profiler::Profiler(ID3D11Device* device, ID3D11DeviceContext * context)
{
	m_device = device;
	m_context = context;
	assert(device);
	assert(context);

	D3D11_QUERY_DESC queryDesc;
	queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	queryDesc.MiscFlags = 0;
	m_device->CreateQuery(&queryDesc, &m_frames[0].disjoint);
	m_device->CreateQuery(&queryDesc, &m_frames[1].disjoint);

	m_isFrameReady = true;
}

void Profiler::StartFrame()
{
	if (m_isFrameReady)
	{
		m_context->Begin(m_frames[m_activeBuffer].disjoint);
	}
}

void Profiler::StartProfiling(std::string profileKey)
{
	if (!m_isFrameReady)
	{
		return;
	}
	if (!m_frames[m_activeBuffer].data.count(profileKey))
	{
		m_frames[m_activeBuffer].data.insert(std::pair<std::string, ClockData>(profileKey, {}));

		D3D11_QUERY_DESC queryDesc;
		queryDesc.Query = D3D11_QUERY_TIMESTAMP;
		queryDesc.MiscFlags = 0;
		m_device->CreateQuery(&queryDesc, &m_frames[m_activeBuffer].data[profileKey].tsStart);
		m_device->CreateQuery(&queryDesc, &m_frames[m_activeBuffer].data[profileKey].tsEnd);
	}
	
	//Create timestamp with END because "start" is disabled 
	//Relate to https://docs.microsoft.com/pl-pl/windows/win32/api/d3d11/ne-d3d11-d3d11_query
	m_context->End(m_frames[m_activeBuffer].data[profileKey].tsStart);
}

void Profiler::EndProfiling(std::string profileKey)
{
	if (!m_isFrameReady)
	{
		return;
	}

	if (!m_frames[m_activeBuffer].data.count(profileKey))
	{
		return;
	}
	m_context->End(m_frames[m_activeBuffer].data[profileKey].tsEnd);
}

void Profiler::ResetProfiling()
{
	if (!m_isFrameReady)
	{
		return;
	}
	{
		std::map<std::string, Profiler::ClockData>::iterator it = m_frames[0].data.begin();
		while (it != m_frames[0].data.end())
		{
			it->second.dataCount = 0;
			it->second.totalTime = 0;
			it++;
		}
	}
	{
		std::map<std::string, Profiler::ClockData>::iterator it = m_frames[1].data.begin();
		while (it != m_frames[1].data.end())
		{
			it->second.dataCount = 0;
			it->second.totalTime = 0;
			it++;
		}
	}
}

void Profiler::EndFrame()
{
	if (!m_isFrameReady)
	{
		return;
	}
	//Create disjoint
	m_context->End(m_frames[m_activeBuffer].disjoint);
	m_activeBuffer = GetBufferToCheckIndex();
	CollectProfilingData();
}

void Profiler::CollectProfilingData()
{
	m_isFrameReady = false;
	
	while (m_context->GetData(m_frames[GetBufferToCheckIndex()].disjoint, NULL, 0, 0) == S_FALSE) 
	{
		continue;
	}
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointTimestampDataQuery;
	m_context->GetData(m_frames[GetBufferToCheckIndex()].disjoint, &disjointTimestampDataQuery, sizeof(disjointTimestampDataQuery), 0);
	if (disjointTimestampDataQuery.Disjoint)
	{
		//Failed to get correct data from timestamp disjoint - discard frame data
		return;
	}
	
	std::map<std::string, Profiler::ClockData>::iterator it = m_frames[GetBufferToCheckIndex()].data.begin();
	const std::map<std::string, Profiler::ClockData>::iterator itEnd = m_frames[GetBufferToCheckIndex()].data.end();
	while (it != itEnd)
	{
		UINT64 startTime = 0.0;
		UINT64 endTime = 0.0;
		m_context->GetData(it->second.tsStart, &startTime, sizeof(UINT64), 0);
		m_context->GetData(it->second.tsEnd, &endTime, sizeof(UINT64), 0);

		//it->second.totalTime += (static_cast<float>(endTime) - static_cast<float>(startTime)) / static_cast<float>(disjointTimestampDataQuery.Frequency) * 1000.0f;
		UINT64 elapsedTime = endTime - startTime;
		UINT64 denominator = disjointTimestampDataQuery.Frequency / 1000;
		it->second.totalTime += static_cast<double>(elapsedTime) / static_cast<double>(denominator);
		it->second.dataCount++;

		it++;
	}

	m_isFrameReady = true;
}
