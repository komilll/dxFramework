#ifndef _RENDER_H_
#define _RENDER_H_

#pragma once
#include "framework.h"
#include "DeviceManager.h"
#include "DirectionalLight.h"
#include "UberBuffer.h"
#include "RenderTexture.h"
#include "ModelDX.h"
#include "ShaderSSAO.h"

using namespace DirectX;
class Renderer 
{
public:
	enum class GBufferType {
		Position, Normal, Depth
	};

public:
	friend class GuiManager;
	Renderer() = default;
	Renderer(std::shared_ptr<DeviceManager> deviceManager);

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void Update();
	void Render();

	void AddCameraPosition(float x, float y, float z);
	void AddCameraPosition(XMFLOAT3 addPos);

	void AddCameraRotation(float x, float y, float z);
	void AddCameraRotation(XMFLOAT3 addRot);

private:
	HRESULT CreateConstantBuffers();
	bool CreateShaders(std::string pixelShaderName, std::string vertexShaderName = "VS_Base.hlsl");
	void CreateViewAndPerspective();
	void MapResourceData();
	void SetConstantBuffers();
	void RenderToBackBuffer(RenderTexture* texture);
	void RenderGBuffer(Renderer::GBufferType type);
	void RenderSSAO();

	std::shared_ptr<DeviceManager> m_deviceManager;

	unsigned int m_indexCount;
	unsigned int m_frameCount;

	typedef struct _constantBufferStruct {
		XMFLOAT4X4 world;
		XMFLOAT4X4 view;
		XMFLOAT4X4 projection;
	} ConstantBufferStruct;
	static_assert((sizeof(ConstantBufferStruct) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

	typedef struct _vertexBufferStruct {
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT2 uv;
	} VertexBufferStruct;

	typedef struct _directionalLightBuffer {
		XMFLOAT3 direction;
		float intensity;
	} DirectionalLightBuffer;
	static_assert((sizeof(DirectionalLightBuffer) % 16) == 0, "DirectionalLightBuffer size must be 16-byte aligned");

	typedef struct _propertyBuffer {
		float roughness;
		XMFLOAT3 directionalLightColor;
	} PropertyBuffer;
	static_assert((sizeof(PropertyBuffer) % 16) == 0, "AdditionalBuffer size must be 16-byte aligned");

	typedef struct _specialBufferSSAOStruct {
		std::array<XMFLOAT4, 64> kernelSample;
		XMFLOAT4X4 projectionMatrix;
	} SpecialBufferSSAOStruct;
	static_assert((sizeof(SpecialBufferSSAOStruct) % 16) == 0, "SpecialBufferSSAO size must be 16-byte aligned");

	//Constant buffers
	ConstantBufferStruct m_constantBufferData;
	DirectionalLightBuffer m_directionalLightBufferData;
	UberBufferStruct m_uberBufferData;
	PropertyBuffer m_propertyBufferData;
	SpecialBufferSSAOStruct m_specialBufferSSAOData;

	ID3D11Buffer* m_vertexBuffer		= NULL;
	ID3D11Buffer* m_indexBuffer			= NULL;
	ID3D11InputLayout* m_inputLayout	= NULL;

	//Buffers
	ID3D11Buffer* m_constantBuffer			= NULL;
	ID3D11Buffer* m_directionalLightBuffer	= NULL;
	ID3D11Buffer* m_uberBuffer				= NULL;
	ID3D11Buffer* m_propertyBuffer			= NULL;
	ID3D11Buffer* m_specialBufferSSAO		= NULL;

	//Camera data
	XMFLOAT3 m_cameraPosition{ 0,0,0 };
	XMFLOAT3 m_cameraPositionStoredInFrame{ 0,0,0 };
	XMFLOAT3 m_cameraRotation{ 0,0,0 };

	//Shader data
	ID3D11SamplerState* m_baseSamplerState		 = NULL;
	ID3D11Resource* m_baseResource				 = NULL;
	ID3D11ShaderResourceView* m_baseResourceView = NULL;

	RenderTexture* m_renderTexture				 = NULL;
	RenderTexture* m_positionBufferTexture		 = NULL;
	RenderTexture* m_normalBufferTexture		 = NULL;
	RenderTexture* m_depthBufferTexture			 = NULL;
	RenderTexture* m_backBufferRenderTexture	 = NULL;
	ModelDX* m_backBufferQuadModel				 = NULL;
	ModelDX* m_bunnyModel						 = NULL;

	//Shader buffers
	ID3D11VertexShader* m_baseVertexShader			= NULL;
	ID3D11VertexShader* m_vertexShaderBackBuffer	= NULL;

	ID3D11PixelShader* m_pixelShaderBunny			= NULL;
	ID3D11PixelShader* m_pixelShaderBackBuffer		= NULL;
	ID3D11PixelShader* m_pixelShaderPositionBuffer	= NULL;
	ID3D11PixelShader* m_pixelShaderNormalBuffer	= NULL;
	ID3D11PixelShader* m_pixelShaderDepthBuffer		= NULL;
	ID3D11PixelShader* m_pixelShaderSSAO			= NULL;

	//Postprocess classes
	ShaderSSAO* m_ssao	= NULL;
};

#endif // !_RENDER_H_