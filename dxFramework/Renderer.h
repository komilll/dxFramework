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
	enum class GBufferType : int {
		Position, Normal, Depth, MAX
	};

	enum class NdfType : int {
		Beckmann, GGX, BlinnPhong, MAX
	};

	enum class GeometryType : int {
		Implicit, Neumann, CookTorrance, Kelemen, Beckmann, GGX, SchlickBeckmann, SchlickGGX, MAX
	};

	enum class FresnelType : int {
		None, Schlick, CookTorrance, MAX
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

	void SaveTextureToFile(RenderTexture * texture, const wchar_t* name);

private:
//DEBUG SETTINGS
	bool FREEZE_CAMERA = false;

//Rendering settings
	NdfType m_ndfType = NdfType::GGX;
	GeometryType m_geometryType = GeometryType::GGX;
	FresnelType m_fresnelType = FresnelType::CookTorrance;

//Buffers and rendering data
	std::shared_ptr<DeviceManager> m_deviceManager;

	unsigned int m_indexCount;
	unsigned int m_frameCount;

	typedef struct _constantBufferStruct {
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	} ConstantBufferStruct;
	static_assert((sizeof(ConstantBufferStruct) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

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
		XMMATRIX projectionMatrix;
		int sampleCount;
		float kernelRadius;
		XMFLOAT2 padding;
	} SpecialBufferSSAOStruct;
	static_assert((sizeof(SpecialBufferSSAOStruct) % 16) == 0, "SpecialBufferSSAO size must be 16-byte aligned");

	typedef struct _specialBufferBRDFStruct {
		int ndfType;
		int geometryType;
		int fresnelType;
		int hasAlbedo;
		int hasNormal;
		int hasRoughness;
		int hasMetallic;
		float roughnessValue;
		float metallicValue;
		float f0;
		XMFLOAT2 padding;
	} SpecialBufferBRDFStruct;
	static_assert((sizeof(SpecialBufferBRDFStruct) % 16) == 0, "SpecialBufferBRDFStruct size must be 16-byte aligned");

	//Constant buffers
	ConstantBufferStruct m_constantBufferData;
	DirectionalLightBuffer m_directionalLightBufferData;
	UberBufferStruct m_uberBufferData;
	PropertyBuffer m_propertyBufferData;
	SpecialBufferSSAOStruct m_specialBufferSSAOData;
	SpecialBufferBRDFStruct m_specialBufferBRDFData;

	ID3D11Buffer* m_vertexBuffer		= NULL;
	ID3D11Buffer* m_indexBuffer			= NULL;
	ID3D11InputLayout* m_inputLayout	= NULL;

	//Buffers
	ID3D11Buffer* m_constantBuffer			= NULL;
	ID3D11Buffer* m_directionalLightBuffer	= NULL;
	ID3D11Buffer* m_uberBuffer				= NULL;
	ID3D11Buffer* m_propertyBuffer			= NULL;
	ID3D11Buffer* m_specialBufferSSAO		= NULL;
	ID3D11Buffer* m_specialBufferBRDF		= NULL;

	//Camera data
	XMFLOAT3 m_cameraPosition{ 0,0,0 };
	XMFLOAT3 m_cameraPositionStoredInFrame{ 0,0,0 };
	XMFLOAT3 m_cameraRotation{ 0,0,0 };

	//Shader data
	ID3D11SamplerState* m_baseSamplerState				= NULL;
	ID3D11Resource* m_baseResource						= NULL;
	ID3D11ShaderResourceView* m_baseResourceView		= NULL;
	ID3D11Resource* m_normalResource					= NULL;
	ID3D11ShaderResourceView* m_normalResourceView		= NULL;
	ID3D11Resource* m_roughnessResource					= NULL;
	ID3D11ShaderResourceView* m_roughnessResourceView	= NULL;
	ID3D11Resource* m_metallicResource					= NULL;
	ID3D11ShaderResourceView* m_metallicResourceView	= NULL;

	RenderTexture* m_renderTexture				 = NULL;
	RenderTexture* m_positionBufferTexture		 = NULL;
	RenderTexture* m_normalBufferTexture		 = NULL;
	RenderTexture* m_depthBufferTexture			 = NULL;
	RenderTexture* m_ssaoBufferTexture			 = NULL;
	RenderTexture* m_backBufferRenderTexture	 = NULL;
	ModelDX* m_backBufferQuadModel				 = NULL;
	ModelDX* m_bunnyModel						 = NULL;

	//Shader buffers
	ID3D11VertexShader* m_baseVertexShader			= NULL;
	ID3D11VertexShader* m_vertexShaderViewPosition	= NULL;
	ID3D11VertexShader* m_vertexShaderBackBuffer	= NULL;

	ID3D11PixelShader* m_pixelShaderBunny			= NULL;
	ID3D11PixelShader* m_pixelShaderBackBuffer		= NULL;
	ID3D11PixelShader* m_pixelShaderPositionBuffer	= NULL;
	ID3D11PixelShader* m_pixelShaderNormalBuffer	= NULL;
	ID3D11PixelShader* m_pixelShaderDepthBuffer		= NULL;
	ID3D11PixelShader* m_pixelShaderSSAO			= NULL;
	ID3D11PixelShader* m_pixelShaderBlurSSAO		= NULL;

	//Postprocess classes
	ShaderSSAO* m_ssao	= NULL;
};

#endif // !_RENDER_H_