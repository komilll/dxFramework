#pragma once
#ifndef _SHADER_SSAO_H_
#define _SHADER_SSAO_H_

#include <array>
#include "framework.h"

using namespace DirectX;

class ShaderSSAO 
{
public:
	ShaderSSAO(ID3D11Device* device);
	ID3D11ShaderResourceView* GetShaderRersourceView() { return m_noiseTextureResourceView; };
	std::array<XMFLOAT4, 64> GetSampleKernel() { return m_sampleKernel; };

private:
	void CalculateSampleKernel();
	void CreateNoiseTexture(ID3D11Device* device);

private:
	std::array<XMFLOAT4, 64> m_sampleKernel{};
	ID3D11Texture2D* m_noiseTexture							= NULL;
	ID3D11ShaderResourceView* m_noiseTextureResourceView	= NULL;
};

#endif // !_SHADER_SSAO_H_
