#include "Renderer.h"
#pragma comment(lib, "d3dcompiler.lib")

#include <d3dcompiler.h>
#include "ModelDX.h"
#include "ShaderSwapper.h"
#include <ScreenGrab.h>
#include <Wincodec.h> 

Renderer::Renderer(std::shared_ptr<DeviceManager> deviceManager)
{
	m_frameCount = 0;
	m_deviceManager = deviceManager;
	m_directionalLightBufferData.intensity = 1.0f;
	//m_directionalLightBufferData.direction = XMFLOAT3{ 0.0f, 1.0f, 1.75f };
	m_directionalLightBufferData.direction = XMFLOAT3{ 0.0f, 0.0f, 1.0f };
	m_propertyBufferData.directionalLightColor = XMFLOAT3{ 1,1,1 };
	m_propertyBufferData.roughness = 0.25f;

	//Presentation of suzanne
	//m_cameraPosition = XMFLOAT3{ -13.0f, 9.5f, 26.5f };
	//m_cameraRotation = XMFLOAT3{ 19.0f, -206.0f, 180.0f};

	//Presentation of spheres
	m_cameraPosition = XMFLOAT3{ 15.0f, 22.0f, -70.0f };
	m_cameraRotation = XMFLOAT3{ 0.0f, 0.0f, 180.0f};

	m_deviceManager->ConfigureSamplerState(&m_baseSamplerState);// , D3D11_TEXTURE_ADDRESS_WRAP, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT);

	m_renderTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice());
	m_positionBufferTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice());
	m_normalBufferTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice());
	m_depthBufferTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice(), DXGI_FORMAT_R16_FLOAT);
	m_ssaoBufferTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice(), DXGI_FORMAT_R32_FLOAT);
	m_backBufferRenderTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice());

	m_backBufferQuadModel = new ModelDX();
	m_backBufferQuadModel->SetFullScreenRectangleModel(m_deviceManager->GetDevice());

	m_bunnyModel = new ModelDX();
	m_bunnyModel->LoadModel("sphere.obj", m_deviceManager->GetDevice());
	m_bunnyModel->m_scale = 10.0f;

	ShaderSwapper::CompileShader("VS_Base.hlsl", "PS_BRDF.hlsl", &m_pixelShaderBunny, &m_baseVertexShader, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("VS_BackBuffer.hlsl", "PS_BackBuffer.hlsl", &m_pixelShaderBackBuffer, &m_vertexShaderBackBuffer, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("VS_ViewSpacePosition.hlsl", "", NULL, &m_vertexShaderViewPosition, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("", "PS_SSAO.hlsl", &m_pixelShaderSSAO, NULL, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("", "PS_SSAOBlur.hlsl", &m_pixelShaderBlurSSAO, NULL, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("", "PS_PositionBuffer.hlsl", &m_pixelShaderPositionBuffer, NULL, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("", "PS_NormalBuffer.hlsl", &m_pixelShaderNormalBuffer, NULL, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("", "PS_DepthBuffer.hlsl", &m_pixelShaderDepthBuffer, NULL, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("VS_Skybox.hlsl", "PS_Skybox.hlsl", &m_pixelShaderSkybox, &m_vertexShaderSkybox, &m_inputLayout, m_deviceManager->GetDevice());

	//Prepare SSAO data
	m_ssao = new ShaderSSAO(m_deviceManager->GetDevice());
	m_specialBufferSSAOData.kernelSample = m_ssao->GetSampleKernel();
	m_specialBufferSSAOData.sampleCount = 16;
	m_specialBufferSSAOData.kernelRadius = 5.0f;

	//Prepare BRDF data
	m_specialBufferBRDFData.ndfType = static_cast<int>(m_ndfType);
	m_specialBufferBRDFData.geometryType = static_cast<int>(m_geometryType);
	m_specialBufferBRDFData.fresnelType = static_cast<int>(m_fresnelType);
	m_specialBufferBRDFData.f0 = 0.92f;
	m_specialBufferBRDFData.metallicValue = 1.0f;
	m_specialBufferBRDFData.roughnessValue = 0.0f;
	m_specialBufferBRDFData.debugType = static_cast<int>(m_debugType);

	m_skyboxModel = new ModelDX();
	m_skyboxModel->LoadModel("cube.obj", m_deviceManager->GetDevice());
	CreateSkyboxTexture();
}

void Renderer::CreateDeviceDependentResources()
{
	CreateConstantBuffers();
}

void Renderer::CreateWindowSizeDependentResources()
{
	CreateViewAndPerspective();
}

void Renderer::Update()
{
	if (m_frameCount == MAXUINT) m_frameCount = 0;
}

void Renderer::Render()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();
	ID3D11RenderTargetView* renderTarget = m_deviceManager->GetRenderTarget();
	ID3D11DepthStencilView* depthStencil = m_deviceManager->GetDepthStencil();

	if (m_baseVertexShader && m_pixelShaderBunny)
	{		
		m_deviceManager->SetBackBufferRenderTarget();
		//Input layout is the same for all vertex shaders for now
		context->IASetInputLayout(m_inputLayout);

		//m_renderTexture->SetAsActiveTarget(context, depthStencil, true, true);

		//Create world matrix
		m_constantBufferData.world = XMMatrixIdentity();
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(m_bunnyModel->m_scale, m_bunnyModel->m_scale, m_bunnyModel->m_scale));
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixTranslation(0.0f, 0.0f, 0.0f));
		m_constantBufferData.world = XMMatrixTranspose(m_constantBufferData.world);

		//RenderGBuffer(Renderer::GBufferType::Position);
		//RenderGBuffer(Renderer::GBufferType::Normal);
		//RenderGBuffer(Renderer::GBufferType::Depth);

		//m_constantBufferData.world = XMMatrixIdentity();
		//RenderSSAO();
		//RenderToBackBuffer(m_normalBufferTexture);
		//return;

		context->VSSetShader(m_baseVertexShader, NULL, 0);
		context->PSSetShader(m_pixelShaderBunny, NULL, 0);

		MapResourceData();
		SetConstantBuffers();

		//if (m_specialBufferBRDF)
		//{
		//	D3D11_MAPPED_SUBRESOURCE mappedResource;
		//	const HRESULT result = context->Map(m_specialBufferBRDF, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		//	if (FAILED(result))
		//		return;

		//	SpecialBufferBRDFStruct* dataPtr = static_cast<SpecialBufferBRDFStruct*>(mappedResource.pData);

		//	dataPtr->ndfType = static_cast<int>(m_ndfType);
		//	dataPtr->geometryType = static_cast<int>(m_geometryType);
		//	dataPtr->fresnelType = static_cast<int>(m_fresnelType);

		//	dataPtr->hasAlbedo = static_cast<int>(m_baseResourceView != NULL);
		//	dataPtr->hasNormal = static_cast<int>(m_normalResourceView != NULL);
		//	dataPtr->hasRoughness = static_cast<int>(m_roughnessResourceView != NULL);
		//	dataPtr->hasMetallic = static_cast<int>(m_metallicResourceView != NULL);

		//	dataPtr->roughnessValue = m_specialBufferBRDFData.roughnessValue;
		//	dataPtr->metallicValue = m_specialBufferBRDFData.metallicValue;
		//	dataPtr->f0 = m_specialBufferBRDFData.f0;

		//	dataPtr->padding = XMFLOAT2{0,0};
		//	context->Unmap(m_specialBufferBRDF, 0);

		//	if (m_specialBufferBRDF) context->PSSetConstantBuffers(13, 1, &m_specialBufferBRDF);
		//}
		if (m_roughnessResourceView) context->PSSetShaderResources(1, 1, &m_roughnessResourceView);
		if (m_normalResourceView) context->PSSetShaderResources(2, 1, &m_normalResourceView);
		if (m_metallicResourceView) context->PSSetShaderResources(3, 1, &m_metallicResourceView);
		if (m_skyboxResourceView) context->PSSetShaderResources(4, 1, &m_skyboxResourceView);

		m_indexCount = m_bunnyModel->Render(context);
		//context->DrawIndexed(m_indexCount, 0, 0);
		//return;
		for (int x = 0; x < 10; ++x)
		{
			for (int y = 4; y < 5; ++y)
			{
				m_constantBufferData.world = XMMatrixIdentity();
				m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(m_bunnyModel->m_scale, m_bunnyModel->m_scale, m_bunnyModel->m_scale));
				m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixTranslation(x * 10.0f, y * 10.0f, 0.0f));
				m_constantBufferData.world = XMMatrixTranspose(m_constantBufferData.world);
				// Lock the constant buffer so it can be written to.
				if (m_constantBuffer)
				{
					D3D11_MAPPED_SUBRESOURCE mappedResource;
					const HRESULT result = context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
					if (FAILED(result))
						return;

					// Get a pointer to the data in the constant buffer.
					ConstantBufferStruct* dataPtr = static_cast<ConstantBufferStruct*>(mappedResource.pData);

					dataPtr->world = m_constantBufferData.world;
					dataPtr->view = m_constantBufferData.view;
					dataPtr->projection = m_constantBufferData.projection;
					context->Unmap(m_constantBuffer, 0);
				}

				if (m_specialBufferBRDF)
				{
					D3D11_MAPPED_SUBRESOURCE mappedResource;
					const HRESULT result = context->Map(m_specialBufferBRDF, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
					if (FAILED(result))
						return;

					SpecialBufferBRDFStruct* dataPtr = static_cast<SpecialBufferBRDFStruct*>(mappedResource.pData);

					dataPtr->ndfType = static_cast<int>(m_ndfType);
					dataPtr->geometryType = static_cast<int>(m_geometryType);
					dataPtr->fresnelType = static_cast<int>(m_fresnelType);

					dataPtr->hasAlbedo = static_cast<int>(m_baseResourceView != NULL);
					dataPtr->hasNormal = static_cast<int>(m_normalResourceView != NULL);
					dataPtr->hasRoughness = static_cast<int>(m_roughnessResourceView != NULL);
					dataPtr->hasMetallic = static_cast<int>(m_metallicResourceView != NULL);

					dataPtr->roughnessValue = max(static_cast<float>(x) * 0.1f, 0.001f);
					dataPtr->metallicValue = static_cast<float>(y) * 0.25f;
					dataPtr->f0 = min(max(m_specialBufferBRDFData.f0, 0.001f), 0.99999f);

					dataPtr->debugType = static_cast<int>(m_debugType);

					dataPtr->padding = 0;
					context->Unmap(m_specialBufferBRDF, 0);

					if (m_specialBufferBRDF) context->PSSetConstantBuffers(13, 1, &m_specialBufferBRDF);
				}

				context->DrawIndexed(m_indexCount, 0, 0);
			}
		}
		DrawSkybox();
		//RenderToBackBuffer(m_renderTexture);
		if (DO_SCREENSHOT_NEXT_FRAME)
		{
			DO_SCREENSHOT_NEXT_FRAME = false;
			std::string filename = "Screenshots/ss_" + Framework::currentDateTime() + ".png";
			std::wstring wFilename{ filename.begin(), filename.end() };
			SaveTextureToFile(m_renderTexture, wFilename.c_str());
		}
	}
}

void Renderer::PrepareScreenshotFrame()
{
	DO_SCREENSHOT_NEXT_FRAME = true;
}

void Renderer::AddCameraPosition(float x, float y, float z)
{
	//if (x != 0 || y != 0 || z != 0)
	{
		m_cameraPositionStoredInFrame.x = x;
		m_cameraPositionStoredInFrame.y = y;
		m_cameraPositionStoredInFrame.z = z;
		CreateViewAndPerspective();
	}
}

void Renderer::AddCameraPosition(XMFLOAT3 addPos)
{
	AddCameraPosition(addPos.x, addPos.y, addPos.z);
}

void Renderer::AddCameraRotation(float x, float y, float z)
{
	//if (x != 0 || y != 0 || z != 0)
	{
		m_cameraRotation.x += x;
		m_cameraRotation.y += y;

		if (m_cameraRotation.y > 360.0f)
			m_cameraRotation.y -= 360.0f;
		else if (m_cameraRotation.y < -360.0f)
			m_cameraRotation.y += 360.0f;
		m_cameraRotation.x = m_cameraRotation.x >= 90.0f ? 89.9f : (m_cameraRotation.x <= -90.0f ? -89.9f : m_cameraRotation.x);

		CreateViewAndPerspective();
	}
}

void Renderer::AddCameraRotation(XMFLOAT3 addRot)
{
	AddCameraPosition(addRot.x, addRot.y, addRot.z);
}

HRESULT Renderer::CreateConstantBuffers()
{
	ID3D11Device* device = m_deviceManager->GetDevice();

	//Create matrix buffer
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.ByteWidth = sizeof(ConstantBufferStruct);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;

	HRESULT result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffer);
	if (FAILED(result))
	{
		return result;
	}

	constantBufferDesc.ByteWidth = sizeof(DirectionalLightBuffer);
	result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_directionalLightBuffer);
	if (FAILED(result))
	{
		return result;
	}

	constantBufferDesc.ByteWidth = sizeof(UberBufferStruct);
	result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_uberBuffer);
	if (FAILED(result))
	{
		return result;
	}

	constantBufferDesc.ByteWidth = sizeof(PropertyBuffer);
	result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_propertyBuffer);
	if (FAILED(result))
	{
		return result;
	}

	constantBufferDesc.ByteWidth = sizeof(SpecialBufferSSAOStruct);
	result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_specialBufferSSAO);
	if (FAILED(result))
	{
		return result;
	}

	constantBufferDesc.ByteWidth = sizeof(SpecialBufferBRDFStruct);
	result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_specialBufferBRDF);
	if (FAILED(result))
	{
		return result;
	}

	return result;
}

bool Renderer::CreateShaders(std::string pixelShaderName, std::string vertexShaderName /* = "VS_Base.hlsl" */)
{
	return true;
	//return ShaderSwapper::CompileAndSwapPixelShader(vertexShaderName, pixelShaderName, &m_pixelShader, &m_vertexShader, &m_inputLayout, m_deviceManager->GetDevice());
}

void Renderer::CreateViewAndPerspective()
{
	const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
	constexpr float conv{ 0.0174532925f };

	//Update camera position for shader buffer
	if (!FREEZE_CAMERA)
	{
		m_uberBufferData.viewerPosition = m_cameraPosition;
	}

	// Create the rotation matrix from the yaw, pitch, and roll values.
	const XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_cameraRotation.x * conv, m_cameraRotation.y * conv, m_cameraRotation.z * conv);

	//Move camera along direction we look at
	if (m_cameraPositionStoredInFrame.x != 0 || m_cameraPositionStoredInFrame.y != 0 || m_cameraPositionStoredInFrame.z != 0)
	{
		const XMMATRIX YrotationMatrix = XMMatrixRotationY(m_cameraRotation.y * conv);
		const XMVECTOR camRight = XMVector3TransformCoord(XMVECTOR{ 1,0,0,0 }, YrotationMatrix);
		const XMVECTOR camForward = XMVector3TransformCoord(XMVECTOR{ 0, 0, 1, 0 }, rotationMatrix);

		const XMVECTOR addPos = camRight * m_cameraPositionStoredInFrame.x + camForward * m_cameraPositionStoredInFrame.z;
		m_cameraPosition.x += addPos.m128_f32[0];
		m_cameraPosition.y += (addPos.m128_f32[1] + m_cameraPositionStoredInFrame.y);
		m_cameraPosition.z += addPos.m128_f32[2];

		m_cameraPositionStoredInFrame = XMFLOAT3{ 0,0,0 };
		if (m_cameraPosition.x == 0.0f && m_cameraPosition.y == 0.0f && m_cameraPosition.z == 0.0f)
		{
			m_cameraPosition.x = FLT_MIN;
		}
	}
	const DirectX::XMVECTOR eye = DirectX::XMVectorSet(m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 0.0f);

	//Setup target (look at object position)
	XMVECTOR target = XMVector3TransformCoord(DirectX::XMVECTOR{ 0, 0, 1, 0 }, rotationMatrix);
	target = XMVector3Normalize(target);
	target = { m_cameraPosition.x + target.m128_f32[0], m_cameraPosition.y + target.m128_f32[1], m_cameraPosition.z + target.m128_f32[2], 0.0f };

	//Create view matrix
	m_constantBufferData.view = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(eye, target, up));

	//Create perspective matrix
	constexpr float FOV = 3.14f / 4.0f;
	const float aspectRatio = m_deviceManager->GetAspectRatio();
	m_constantBufferData.projection = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(FOV, aspectRatio, 0.01f, 100.0f));
	//Store projection matrix in SSAO as soon as it changes
	//m_specialBufferSSAOData.projectionMatrix = m_constantBufferData.projection;
}

void Renderer::MapResourceData()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	// Lock the constant buffer so it can be written to.
	if (m_constantBuffer)
	{
		const HRESULT result = context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		// Get a pointer to the data in the constant buffer.
		ConstantBufferStruct* dataPtr = static_cast<ConstantBufferStruct*>(mappedResource.pData);

		dataPtr->world = m_constantBufferData.world;
		dataPtr->view = m_constantBufferData.view;
		dataPtr->projection = m_constantBufferData.projection;
		context->Unmap(m_constantBuffer, 0);
	}

	//MAP DIRECTIONAL LIGHT DATA
	if (m_directionalLightBuffer)
	{
		const HRESULT result = context->Map(m_directionalLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		DirectionalLightBuffer* dataPtr = static_cast<DirectionalLightBuffer*>(mappedResource.pData);

		dataPtr->direction = m_directionalLightBufferData.direction;
		dataPtr->intensity = m_directionalLightBufferData.intensity;
		context->Unmap(m_directionalLightBuffer, 0);
	}

	//MAP ADDITIONAL DATA
	if (m_uberBuffer)
	{
		const HRESULT result = context->Map(m_uberBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		UberBufferStruct* dataPtr = static_cast<UberBufferStruct*>(mappedResource.pData);

		dataPtr->viewerPosition = m_uberBufferData.viewerPosition;
		dataPtr->padding = m_uberBufferData.padding;
		context->Unmap(m_uberBuffer, 0);
	}

	//MAP PROPERTY DATA
	if (m_propertyBuffer)
	{
		const HRESULT result = context->Map(m_propertyBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		PropertyBuffer* dataPtr = static_cast<PropertyBuffer*>(mappedResource.pData);

		dataPtr->roughness = m_propertyBufferData.roughness;
		dataPtr->directionalLightColor = m_propertyBufferData.directionalLightColor;
		context->Unmap(m_propertyBuffer, 0);
	}
}

void Renderer::SetConstantBuffers()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	if (m_constantBuffer) context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
	if (m_directionalLightBuffer) context->VSSetConstantBuffers(1, 1, &m_directionalLightBuffer);
	if (m_uberBuffer) context->VSSetConstantBuffers(7, 1, &m_uberBuffer);
	if (m_propertyBuffer) context->PSSetConstantBuffers(0, 1, &m_propertyBuffer);

	if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
	if (m_baseResourceView) context->PSSetShaderResources(0, 1, &m_baseResourceView);
}

void Renderer::DrawSkybox()
{
	if (m_skyboxModel && m_pixelShaderSkybox && m_vertexShaderSkybox)
	{
		ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

		m_deviceManager->UseSkyboxDepthStencilStateAndRasterizer();

		m_skyboxModel->m_position = XMFLOAT3{ m_cameraPosition.x - 0.5f, m_cameraPosition.y - 0.5f, m_cameraPosition.z - 0.5f };
		m_constantBufferData.world = XMMatrixIdentity();
		//m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(m_skyboxModel->m_scale, m_skyboxModel->m_scale, m_skyboxModel->m_scale));
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixTranslation(m_skyboxModel->m_position.x, m_skyboxModel->m_position.y, m_skyboxModel->m_position.z));
		m_constantBufferData.world = XMMatrixTranspose(m_constantBufferData.world);
		MapResourceData();

		if (m_constantBuffer) context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
		if (m_constantBuffer) context->PSSetConstantBuffers(7, 1, &m_uberBuffer);

		context->IASetInputLayout(m_inputLayout);
		context->VSSetShader(m_vertexShaderSkybox, NULL, 0);
		context->PSSetShader(m_pixelShaderSkybox, NULL, 0);

		if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
		if (m_skyboxResourceView) context->PSSetShaderResources(0, 1, &m_skyboxResourceView);

		m_indexCount = m_skyboxModel->Render(context);
		context->DrawIndexed(m_indexCount, 0, 0);

		m_deviceManager->UseStandardDepthStencilStateAndRasterizer();
		ID3D11ShaderResourceView* emptyView = nullptr;
		context->PSSetShaderResources(0, 1, &emptyView);
	}
}

bool Renderer::CreateSkyboxTexture()
{
	std::array<ID3D11Resource*, 6> textures{ nullptr };
	std::array<ID3D11ShaderResourceView*, 6> textureViews{ nullptr };

	bool loadingTextures = SUCCEEDED(m_deviceManager->LoadTextureFromFile(L"Resources/Skyboxes/posx.jpg", &textures[0], &textureViews[0]));
	loadingTextures &=	SUCCEEDED(m_deviceManager->LoadTextureFromFile(L"Resources/Skyboxes/negx.jpg", &textures[1], &textureViews[1]));
	loadingTextures &= SUCCEEDED(m_deviceManager->LoadTextureFromFile(L"Resources/Skyboxes/posy.jpg", &textures[2], &textureViews[2]));
	loadingTextures &= SUCCEEDED(m_deviceManager->LoadTextureFromFile(L"Resources/Skyboxes/negy.jpg", &textures[3], &textureViews[3]));
	loadingTextures &= SUCCEEDED(m_deviceManager->LoadTextureFromFile(L"Resources/Skyboxes/posz.jpg", &textures[4], &textureViews[4]));
	loadingTextures &= SUCCEEDED(m_deviceManager->LoadTextureFromFile(L"Resources/Skyboxes/negz.jpg", &textures[5], &textureViews[5]));

	if (!loadingTextures)
	{
		return false;
	}
	D3D11_TEXTURE2D_DESC importedTexDesc;
	((ID3D11Texture2D*)textures[0])->GetDesc(&importedTexDesc);

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = importedTexDesc.Width;
	texDesc.Height = importedTexDesc.Height;
	texDesc.MipLevels = importedTexDesc.MipLevels;
	texDesc.ArraySize = 6;
	texDesc.Format = importedTexDesc.Format;
	texDesc.CPUAccessFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	ID3D11Texture2D* texArray = 0;
	if (FAILED(m_deviceManager->GetDevice()->CreateTexture2D(&texDesc, 0, &texArray)))
		return false;

	// Copy individual texture elements into texture array.
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();
	D3D11_BOX sourceRegion;

	//Here i copy the mip map levels of the textures
	for (UINT i = 0; i < 6; ++i)
	{
		for (UINT mipLevel = 0; mipLevel < texDesc.MipLevels; mipLevel++)
		{
			sourceRegion.left = 0;
			sourceRegion.right = (texDesc.Width >> mipLevel);
			sourceRegion.top = 0;
			sourceRegion.bottom = (texDesc.Height >> mipLevel);
			sourceRegion.front = 0;
			sourceRegion.back = 1;

			//test for overflow
			if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
				break;

			context->CopySubresourceRegion(texArray, D3D11CalcSubresource(mipLevel, i, texDesc.MipLevels), 0, 0, 0, textures[i], mipLevel, &sourceRegion);
		}
	}

	// Create a resource view to the texture array.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	viewDesc.TextureCube.MostDetailedMip = 0;
	viewDesc.TextureCube.MipLevels = texDesc.MipLevels;

	if (FAILED(m_deviceManager->GetDevice()->CreateShaderResourceView(texArray, &viewDesc, &m_skyboxResourceView)))
		return false;

	return true;
}

void Renderer::RenderToBackBuffer(RenderTexture * texture)
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	m_deviceManager->SetBackBufferRenderTarget();
	m_indexCount = m_backBufferQuadModel->Render(context);
	
	context->VSSetShader(m_vertexShaderBackBuffer, NULL, 0);
	context->PSSetShader(m_pixelShaderBackBuffer, NULL, 0);

	if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
	if (texture)
	{
		auto texView = texture->GetResourceView();
		if (texView)
		{
			context->PSSetShaderResources(0, 1, &texView);
		}
	}

	context->DrawIndexed(m_indexCount, 0, 0);
}

void Renderer::RenderGBuffer(Renderer::GBufferType type)
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	switch (type)
	{
	case Renderer::GBufferType::Position:
		m_positionBufferTexture->SetAsActiveTarget(context, m_deviceManager->GetDepthStencil(), true, true, XMFLOAT4{ 0,0,0,0 });
		context->VSSetShader(m_vertexShaderViewPosition, NULL, 0);
		context->PSSetShader(m_pixelShaderPositionBuffer, NULL, 0);
		break;

	case Renderer::GBufferType::Normal:
		m_normalBufferTexture->SetAsActiveTarget(context, m_deviceManager->GetDepthStencil(), true, true, XMFLOAT4{ 0,0,0,0 });
		context->VSSetShader(m_baseVertexShader, NULL, 0);
		context->PSSetShader(m_pixelShaderNormalBuffer, NULL, 0);
		break;

	case Renderer::GBufferType::Depth:
		m_depthBufferTexture->SetAsActiveTarget(context, m_deviceManager->GetDepthStencil(), true, true, XMFLOAT4{ 0,0,0,0 });
		context->VSSetShader(m_baseVertexShader, NULL, 0);
		context->PSSetShader(m_pixelShaderDepthBuffer, NULL, 0);
		break;

	default:
		return;
	}

	MapResourceData();
	SetConstantBuffers();
	m_indexCount = m_bunnyModel->Render(context);
	context->DrawIndexed(m_indexCount, 0, 0);
}

void Renderer::RenderSSAO()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	m_ssaoBufferTexture->SetAsActiveTarget(context, m_deviceManager->GetDepthStencil(), true, false, XMFLOAT4{ 1,1,1,1 });
	//m_deviceManager->SetBackBufferRenderTarget();
	m_indexCount = m_backBufferQuadModel->Render(context);

	context->VSSetShader(m_vertexShaderBackBuffer, NULL, 0);
	context->PSSetShader(m_pixelShaderSSAO, NULL, 0);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (m_constantBuffer)
	{
		const HRESULT result = context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		// Get a pointer to the data in the constant buffer.
		ConstantBufferStruct* dataPtr = static_cast<ConstantBufferStruct*>(mappedResource.pData);

		dataPtr->world = m_constantBufferData.world;
		dataPtr->view = m_constantBufferData.view;
		dataPtr->projection = m_constantBufferData.projection;
		context->Unmap(m_constantBuffer, 0);

		context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
	}

	//MAP ADDITIONAL DATA
	if (m_uberBuffer)
	{
		const HRESULT result = context->Map(m_uberBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		UberBufferStruct* dataPtr = static_cast<UberBufferStruct*>(mappedResource.pData);

		dataPtr->viewerPosition = m_uberBufferData.viewerPosition;
		dataPtr->padding = m_uberBufferData.padding;
		context->Unmap(m_uberBuffer, 0);

		context->VSSetConstantBuffers(7, 1, &m_uberBuffer);
	}

	if (m_specialBufferSSAO)
	{
		const HRESULT result = context->Map(m_specialBufferSSAO, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		SpecialBufferSSAOStruct* dataPtr = static_cast<SpecialBufferSSAOStruct*>(mappedResource.pData);

		dataPtr->kernelSample = m_specialBufferSSAOData.kernelSample;
		dataPtr->projectionMatrix = m_specialBufferSSAOData.projectionMatrix;
		dataPtr->kernelRadius = m_specialBufferSSAOData.kernelRadius;
		dataPtr->sampleCount = m_specialBufferSSAOData.sampleCount;
		dataPtr->padding = XMFLOAT2{ 0,0 };
		context->Unmap(m_specialBufferSSAO, 0);

		context->PSSetConstantBuffers(13, 1, &m_specialBufferSSAO);
	}
	if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
	auto noiseTex = m_ssao->GetShaderRersourceView();
	//auto depthTex = m_depthBufferTexture->GetResourceView();
	auto positionTex = m_positionBufferTexture->GetResourceView();
	auto normalTex = m_normalBufferTexture->GetResourceView();

	context->PSSetShaderResources(0, 1, &noiseTex);
	context->PSSetShaderResources(1, 1, &positionTex);
	context->PSSetShaderResources(2, 1, &normalTex);

	context->DrawIndexed(m_indexCount, 0, 0);

	/* Blur SSAO */
	m_deviceManager->SetBackBufferRenderTarget();

	m_indexCount = m_backBufferQuadModel->Render(context);

	context->VSSetShader(m_vertexShaderBackBuffer, NULL, 0);
	context->PSSetShader(m_pixelShaderBlurSSAO, NULL, 0);

	auto ssaoTex = m_ssaoBufferTexture->GetResourceView();
	if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
	context->PSSetShaderResources(0, 1, &ssaoTex);

	context->DrawIndexed(m_indexCount, 0, 0);
}

void Renderer::SaveTextureToFile(RenderTexture * texture, const wchar_t* name)
{
	if (texture && m_deviceManager->GetDeviceContext())
	{
		ID3D11Resource* resource;
		texture->GetResourceView()->GetResource(&resource);
		const HRESULT result = SaveWICTextureToFile(m_deviceManager->GetDeviceContext(), resource, GUID_ContainerFormatPng, name);
		if (FAILED(result))	{
			assert(false);
		}
	}
}
