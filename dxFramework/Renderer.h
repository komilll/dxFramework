#ifndef _RENDER_H_
#define _RENDER_H_

#pragma once
#include "framework.h"
#include "DeviceManager.h"
#include "UberBuffer.h"
#include "RenderTexture.h"
#include "ModelDX.h"
#include "ShaderSSAO.h"
#include "Profiler.h"
#include "BaseLight.h"

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

	enum class DebugType : int {
		None, Diffuse, Specular, Albedo, Normal, Roughness, Metallic, NormalDistribution, GeomtetryShadowing, Fresnel, MAX
	};

public:
	friend class GuiManager;
	friend class Main;
	Renderer() = default;
	Renderer(std::shared_ptr<DeviceManager> deviceManager);

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void Update();
	void Render();
	void PrepareScreenshotFrame();

	void AddCameraPosition(float x, float y, float z);
	void AddCameraPosition(XMFLOAT3 addPos);

	void AddCameraRotation(float x, float y, float z);
	void AddCameraRotation(XMFLOAT3 addRot);

public:
	//CONST VARIABLES
	static const unsigned int SPECULAR_CONVOLUTION_MIPS = 6;
	static_assert(SPECULAR_CONVOLUTION_MIPS >= 1 && SPECULAR_CONVOLUTION_MIPS <= 9, "Mip levels must be between 1 and 9 inclusively");

private:
	HRESULT CreateConstantBuffers();
	bool CreateShaders(std::string pixelShaderName, std::string vertexShaderName = "VS_Base.hlsl");
	void CreateViewAndPerspective();
	void MapResourceData();
	void SetConstantBuffers();

	void DrawSkybox();
	bool ConvoluteDiffuseSkybox();
	void ConvoluteSpecularSkybox();
	void PrecomputeEnvironmentBRDF();
	bool ConstructCubemap(std::array<std::wstring, 6> textureNames, ID3D11ShaderResourceView** cubemapView);
	bool ConstructCubemapFromTextures(std::array<RenderTexture*, SPECULAR_CONVOLUTION_MIPS * 6> textureFaces, ID3D11ShaderResourceView** cubemapView);
	bool CreateSkyboxCubemap();
	bool CreateDiffuseCubemapIBL();

	void RenderToBackBuffer(RenderTexture* texture);
	void RenderGBuffer(Renderer::GBufferType type);
	void RenderSSAO();

	void SaveTextureToFile(RenderTexture * texture, const wchar_t* name);

private:
//DEBUG SETTINGS
	bool FREEZE_CAMERA = false;
	bool DO_SCREENSHOT_NEXT_FRAME = false;
	Profiler* m_profiler;

//SCENE SETTINGS

//Rendering settings
	NdfType m_ndfType = NdfType::GGX;
	GeometryType m_geometryType = GeometryType::GGX;
	FresnelType m_fresnelType = FresnelType::CookTorrance;
	DebugType m_debugType = DebugType::None;

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
		int debugType;

		float padding;
	} SpecialBufferBRDFStruct;
	static_assert((sizeof(SpecialBufferBRDFStruct) % 16) == 0, "SpecialBufferBRDFStruct size must be 16-byte aligned");

	typedef struct _specialBufferPrecomputeIBLStruct {
		int cubemapFaceIndex;
		float roughness;
		XMFLOAT2 padding;
	} SpecialBufferPrecomputeIBLStruct;
	static_assert((sizeof(SpecialBufferPrecomputeIBLStruct) % 16) == 0, "SpecialBufferPrecomputeIBLStruct size must be 16-byte aligned");

	//Constant buffers
	ConstantBufferStruct m_constantBufferData;
	UberBufferStruct m_uberBufferData;
	PropertyBuffer m_propertyBufferData;
	SpecialBufferSSAOStruct m_specialBufferSSAOData;
	SpecialBufferBRDFStruct m_specialBufferBRDFData;
	SpecialBufferPrecomputeIBLStruct m_specialBufferPrecomputeIBLData;

	ID3D11Buffer* m_vertexBuffer		= NULL;
	ID3D11Buffer* m_indexBuffer			= NULL;
	ID3D11InputLayout* m_inputLayout	= NULL;

	//Buffers
	ID3D11Buffer* m_constantBuffer			   = NULL;
	ID3D11Buffer* m_uberBuffer				   = NULL;
	ID3D11Buffer* m_propertyBuffer			   = NULL;
	ID3D11Buffer* m_specialBufferSSAO		   = NULL;
	ID3D11Buffer* m_specialBufferBRDF		   = NULL;
	ID3D11Buffer* m_specialBufferPrecomputeIBL = NULL;

	//Camera data
	XMFLOAT3 m_cameraPosition{ 0,0,0 };
	XMFLOAT3 m_cameraPositionStoredInFrame{ 0,0,0 };
	XMFLOAT3 m_cameraRotation{ 0,0,0 };

	//Shader data
	ID3D11SamplerState* m_baseSamplerState				= NULL;
	ID3D11ShaderResourceView* m_skyboxResourceView		= NULL;
	ID3D11ShaderResourceView* m_diffuseIBLResourceView  = NULL;
	ID3D11ShaderResourceView* m_specularIBLResourceView = NULL;
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
	RenderTexture* m_diffuseConvolutionTexture	 = NULL;
	RenderTexture* m_environmentBRDF			 = NULL;
	std::array<RenderTexture*, SPECULAR_CONVOLUTION_MIPS * 6> m_specularConvolutionTexture;
	ModelDX* m_backBufferQuadModel				 = NULL;
	ModelDX* m_bunnyModel						 = NULL;
	ModelDX* m_skyboxModel						 = NULL;

	//Shader buffers
	ID3D11VertexShader* m_baseVertexShader			= NULL;
	ID3D11VertexShader* m_vertexShaderViewPosition	= NULL;
	ID3D11VertexShader* m_vertexShaderBackBuffer	= NULL;
	ID3D11VertexShader* m_vertexShaderSkybox		= NULL;

	ID3D11PixelShader* m_pixelShaderBunny			= NULL;
	ID3D11PixelShader* m_pixelShaderBackBuffer		= NULL;
	ID3D11PixelShader* m_pixelShaderPositionBuffer	= NULL;
	ID3D11PixelShader* m_pixelShaderNormalBuffer	= NULL;
	ID3D11PixelShader* m_pixelShaderDepthBuffer		= NULL;
	ID3D11PixelShader* m_pixelShaderSSAO			= NULL;
	ID3D11PixelShader* m_pixelShaderBlurSSAO		= NULL;
	ID3D11PixelShader* m_pixelShaderSkybox			= NULL;
	ID3D11PixelShader* m_pixelShaderDiffuseIBL		= NULL;
	ID3D11PixelShader* m_pixelShaderSpecularIBL     = NULL;
	ID3D11PixelShader* m_pixelShaderEnvironmentBRDF = NULL;

	//Postprocess classes
	ShaderSSAO* m_ssao	= NULL;
};

#endif // !_RENDER_H_