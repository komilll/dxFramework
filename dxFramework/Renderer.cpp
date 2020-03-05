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
	m_directionalLightBufferData.direction = XMFLOAT3{ 0.0f, 1.0f, 1.75f };
	m_propertyBufferData.directionalLightColor = XMFLOAT3{ 1,1,1 };
	m_propertyBufferData.roughness = 0.25f;

	m_cameraPosition = XMFLOAT3{ 0.5f, 1.5f, -6.f};

	m_deviceManager->ConfigureSamplerState(&m_baseSamplerState);// , D3D11_TEXTURE_ADDRESS_WRAP, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT);

	m_renderTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice());
	m_positionBufferTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice());
	m_normalBufferTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice());
	m_depthBufferTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice(), DXGI_FORMAT_R16_FLOAT);	
	m_backBufferRenderTexture = new RenderTexture(1280, 720, m_deviceManager->GetDevice());

	m_backBufferQuadModel = new ModelDX();
	m_backBufferQuadModel->SetFullScreenRectangleModel(m_deviceManager->GetDevice());

	m_bunnyModel = new ModelDX();
	m_bunnyModel->LoadModel("bunny.obj", m_deviceManager->GetDevice());
	m_bunnyModel->m_scale = 100.0f;

	ShaderSwapper::CompileShader("VS_Base.hlsl", "PS_BlinnPhong.hlsl", &m_pixelShaderBunny, &m_baseVertexShader, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("VS_BackBuffer.hlsl", "PS_BackBuffer.hlsl", &m_pixelShaderBackBuffer, &m_vertexShaderBackBuffer, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("", "PS_SSAO.hlsl", &m_pixelShaderSSAO, NULL, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("", "PS_PositionBuffer.hlsl", &m_pixelShaderPositionBuffer, NULL, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("", "PS_NormalBuffer.hlsl", &m_pixelShaderNormalBuffer, NULL, &m_inputLayout, m_deviceManager->GetDevice());
	ShaderSwapper::CompileShader("", "PS_DepthBuffer.hlsl", &m_pixelShaderDepthBuffer, NULL, &m_inputLayout, m_deviceManager->GetDevice());

	m_ssao = new ShaderSSAO(m_deviceManager->GetDevice());
	m_specialBufferSSAOData.kernelSample = m_ssao->GetSampleKernel();
}

void Renderer::CreateDeviceDependentResources()
{
	//CreateShaders("PS_BlinnPhong.hlsl");
	//CreateShaders("PS_Lambert.hlsl");
	CreateConstantBuffers();
	//SetModelTarget(m_bunnyModel);
}

void Renderer::CreateWindowSizeDependentResources()
{
	CreateViewAndPerspective();
}

void Renderer::Update()
{
	//DirectX::XMStoreFloat4x4(
	//	&m_constantBufferData.world,
	//	DirectX::XMMatrixTranspose(
	//		DirectX::XMMatrixRotationY(
	//			DirectX::XMConvertToRadians(
	//				static_cast<float>(m_frameCount++)
	//			)
	//		)
	//	)
	//);

	if (m_frameCount == MAXUINT) m_frameCount = 0;
}

void Renderer::Render()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();
	ID3D11RenderTargetView* renderTarget = m_deviceManager->GetRenderTarget();
	ID3D11DepthStencilView* depthStencil = m_deviceManager->GetDepthStencil();

	if (m_baseVertexShader && m_pixelShaderBunny)
	{		
		//m_deviceManager->SetBackBufferRenderTarget();
		//Input layout is the same for all vertex shaders for now
		context->IASetInputLayout(m_inputLayout);

		//m_renderTexture->SetAsActiveTarget(context, depthStencil, true, true);

		//Create world matrix
		DirectX::XMStoreFloat4x4(&m_constantBufferData.world, XMMatrixIdentity() * XMMatrixScaling(m_bunnyModel->m_scale, m_bunnyModel->m_scale, m_bunnyModel->m_scale));
		MapResourceData();
		SetConstantBuffers();

		RenderGBuffer(Renderer::GBufferType::Position);
		RenderGBuffer(Renderer::GBufferType::Normal);
		RenderGBuffer(Renderer::GBufferType::Depth);
		RenderSSAO();
		//RenderToBackBuffer(m_normalBufferTexture);
		return;

		context->VSSetShader(m_baseVertexShader, NULL, 0);
		context->PSSetShader(m_pixelShaderBunny, NULL, 0);

		SetConstantBuffers();

		m_indexCount = m_bunnyModel->Render(context);
		context->DrawIndexed(m_indexCount, 0, 0);

		RenderToBackBuffer(m_renderTexture);
		//static bool doOnce = false;
		//if (doOnce)
		//{
		//	if (m_baseResource && m_deviceManager->GetDeviceContext())
		//	{
		//		doOnce = false;
		//		ID3D11Resource* resource;
		//		m_renderTexture->GetResourceView()->GetResource(&resource);
		//		const HRESULT result = SaveWICTextureToFile(context, resource, GUID_ContainerFormatPng, L"SCREENSHOT.JPG");
		//		if (FAILED(result))	{
		//			assert(false);
		//		}
		//	}
		//}
	}
}

void Renderer::AddCameraPosition(float x, float y, float z)
{
	if (x != 0 || y != 0 || z != 0)
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
	if (x != 0 || y != 0 || z != 0)
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
	m_uberBufferData.viewerPosition = m_cameraPosition;

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
	DirectX::XMStoreFloat4x4(
		&m_constantBufferData.view,
		DirectX::XMMatrixTranspose(
			DirectX::XMMatrixLookAtLH(
				eye,
				target,
				up
			)
		)
	);

	//Create perspective matrix
	DirectX::XMStoreFloat4x4(
		&m_constantBufferData.projection,
		DirectX::XMMatrixTranspose(
			DirectX::XMMatrixPerspectiveFovLH(
				DirectX::XMConvertToRadians(45),
				m_deviceManager->GetAspectRatio(),
				0.01f,
				100.0f
			)
		)
	);
	//Store projection matrix in SSAO as soon as it changes
	m_specialBufferSSAOData.projectionMatrix = m_constantBufferData.projection;
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
		context->VSSetShader(m_baseVertexShader, NULL, 0);
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

	m_indexCount = m_bunnyModel->Render(context);
	context->DrawIndexed(m_indexCount, 0, 0);
}

void Renderer::RenderSSAO()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	m_deviceManager->SetBackBufferRenderTarget();
	m_indexCount = m_backBufferQuadModel->Render(context);

	context->VSSetShader(m_vertexShaderBackBuffer, NULL, 0);
	context->PSSetShader(m_pixelShaderSSAO, NULL, 0);

	if (m_specialBufferSSAO)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		const HRESULT result = context->Map(m_specialBufferSSAO, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		SpecialBufferSSAOStruct* dataPtr = static_cast<SpecialBufferSSAOStruct*>(mappedResource.pData);

		dataPtr->kernelSample = m_specialBufferSSAOData.kernelSample;
		dataPtr->projectionMatrix = m_specialBufferSSAOData.projectionMatrix;
		context->Unmap(m_specialBufferSSAO, 0);

		context->PSSetConstantBuffers(13, 1, &m_specialBufferSSAO);
	}
	if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
	auto noiseTex = m_ssao->GetShaderRersourceView();
	auto depthTex = m_depthBufferTexture->GetResourceView();
	auto normalTex = m_normalBufferTexture->GetResourceView();

	context->PSSetShaderResources(0, 1, &noiseTex);
	context->PSSetShaderResources(1, 1, &depthTex);
	context->PSSetShaderResources(2, 1, &normalTex);

	context->DrawIndexed(m_indexCount, 0, 0);
}
