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
	HRESULT result;

	const std::uniform_real_distribution<float> randomFloat{ -1.0f, 1.0f };
	std::default_random_engine generator;
	constexpr unsigned int width = 4;
	constexpr unsigned int height = 4;
	std::array<XMFLOAT2, width * height> noiseData;

	for (size_t i = 0; i < noiseData.size(); ++i)
	{
		noiseData[i] = XMFLOAT2{ randomFloat(generator), randomFloat(generator) };
	}

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &noiseData;
	data.SysMemPitch = width * 8;
	data.SysMemSlicePitch = width * height * 8;

	result = device->CreateTexture2D(&textureDesc, &data, &m_noiseTexture);
	if (FAILED(result))
	{
		assert(false);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	result = device->CreateShaderResourceView(m_noiseTexture, &shaderResourceViewDesc, &m_noiseTextureResourceView);
	if (FAILED(result))
	{
		assert(false);
	}
}