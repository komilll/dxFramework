#include "ShaderSSAO.h"
#include <random>

ShaderSSAO::ShaderSSAO(ID3D11Device * device)
{
	CalculateSampleKernel();
	CreateNoiseTexture(device);
}

void ShaderSSAO::CalculateSampleKernel()
{
	const std::uniform_real_distribution<float> randomFloatsXY{ -1.0f, 1.0f };
	const std::uniform_real_distribution<float> randomFloatsZ{ 0.0f, 1.0f };
	std::default_random_engine generator;

	for (int i = 0; i < m_sampleKernel.size(); i++)
	{
		//Generate random vector3 ([-1, 1], [-1, 1], [0, 1])
		XMVECTOR tmpVector = { randomFloatsXY(generator), randomFloatsXY(generator), randomFloatsZ(generator) };
		//Normalize vector3
		tmpVector = XMVector3Normalize(tmpVector);

		//Scale samples so they are more aligned to middle of hemisphere
		float scale = float(i) / 64.0f;
		scale = Framework::lerp(0.1f, 1.0f, scale * scale);
		m_sampleKernel[i].x = tmpVector.m128_f32[0] * scale;
		m_sampleKernel[i].y = tmpVector.m128_f32[1] * scale;
		m_sampleKernel[i].z = tmpVector.m128_f32[2] * scale;
		m_sampleKernel[i].w = 0.0f;
	}
}

void ShaderSSAO::CreateNoiseTexture(ID3D11Device * device)
{

}
