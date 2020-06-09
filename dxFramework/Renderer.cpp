#include "Renderer.h"
#pragma comment(lib, "d3dcompiler.lib")

#include <d3dcompiler.h>
#include "ModelDX.h"
#include "ShaderSwapper.h"
#include <ScreenGrab.h>
#include <Wincodec.h> 
#include <sstream>

Renderer::Renderer(std::shared_ptr<DeviceManager> deviceManager)
{
	m_frameCount = 0;
	m_deviceManager = deviceManager;
	ID3D11Device* device = m_deviceManager->GetDevice();

	//m_directionalLightBufferData.direction = XMFLOAT3{ 0.0f, 1.0f, 1.75f };
	m_uberBufferData.directionalLightDirection = XMFLOAT3{ 0.0f, 0.0f, 1.0f };
	m_uberBufferData.directionalLightColor = XMFLOAT4{ 1,1,1,0 };
	m_propertyBufferData.roughness = 0.25f;

	//Presentation of suzanne
	//m_cameraPosition = XMFLOAT3{ -13.0f, 9.5f, 26.5f };
	//m_cameraRotation = XMFLOAT3{ 19.0f, -206.0f, 180.0f};

	//Presentation of spheres
	//m_cameraPosition = XMFLOAT3{ 15.0f, 22.0f, -70.0f };
	//m_cameraRotation = XMFLOAT3{ 0.0f, 0.0f, 180.0f};

	m_cameraPosition = XMFLOAT3{ 15.0f, 50.0f, -85.0f };
	m_cameraRotation = XMFLOAT3{ 45.0f, 0.0f, 180.0f};

	m_deviceManager->ConfigureSamplerState(&m_baseSamplerState);// , D3D11_TEXTURE_ADDRESS_WRAP, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT);

	m_renderTexture = new RenderTexture(1280, 720, device);
	m_positionBufferTexture = new RenderTexture(1280, 720, device);
	m_normalBufferTexture = new RenderTexture(1280, 720, device);
	m_depthBufferTexture = new RenderTexture(1280, 720, device, false, DXGI_FORMAT_R16_FLOAT);
	m_ssaoBufferTexture = new RenderTexture(1280, 720, device, false, DXGI_FORMAT_R32_FLOAT);
	m_backBufferRenderTexture = new RenderTexture(1280, 720, device);
	m_diffuseConvolutionTexture = new RenderTexture(256, 256, device);
	m_environmentBRDF = new RenderTexture(256, 256, device, false, DXGI_FORMAT_R32G32_FLOAT);
	m_shadowMapTexture = new RenderTexture(1024, 1024, device, true);
	for (auto& texture : m_specularConvolutionTexture) { texture = new RenderTexture(256, 256, device); }
	
	m_backBufferQuadModel = new ModelDX();
	m_backBufferQuadModel->SetFullScreenRectangleModel(device);

	m_bunnyModel = new ModelDX();
	m_bunnyModel->LoadModel("bunny.obj", device);
	m_bunnyModel->m_scale = 3.5f;

	m_sphereModel = new ModelDX();
	m_sphereModel->LoadModel("sphere.obj", device);
	m_sphereModel->m_scale = 0.08f;

	m_groundPlaneModel = new ModelDX();
	m_groundPlaneModel->CreatePlane(device, { 25, 25 });
	m_groundPlaneModel->m_rotation = XMFLOAT3{ 89.9f * 0.0174532925f, 0, 0 };
	m_groundPlaneModel->m_position = XMFLOAT3{ 15.0f, 22.0f, -50.0f };

	m_profiler = new Profiler(device, m_deviceManager->GetDeviceContext());

	ShaderSwapper::CompileShader("VS_Base.hlsl", "PS_BRDF.hlsl", &m_pixelShaderBunny, &m_baseVertexShader, &m_inputLayout, device);
	ShaderSwapper::CompileShader("VS_BackBuffer.hlsl", "PS_BackBuffer.hlsl", &m_pixelShaderBackBuffer, &m_vertexShaderBackBuffer, &m_inputLayout, device);
	ShaderSwapper::CompileShader("VS_ViewSpacePosition.hlsl", "", NULL, &m_vertexShaderViewPosition, &m_inputLayout, device);
	ShaderSwapper::CompileShader("", "PS_SSAO.hlsl", &m_pixelShaderSSAO, NULL, &m_inputLayout, device);
	ShaderSwapper::CompileShader("", "PS_SSAOBlur.hlsl", &m_pixelShaderBlurSSAO, NULL, &m_inputLayout, device);
	ShaderSwapper::CompileShader("", "PS_PositionBuffer.hlsl", &m_pixelShaderPositionBuffer, NULL, &m_inputLayout, device);
	ShaderSwapper::CompileShader("", "PS_NormalBuffer.hlsl", &m_pixelShaderNormalBuffer, NULL, &m_inputLayout, device);
	ShaderSwapper::CompileShader("", "PS_DepthBuffer.hlsl", &m_pixelShaderDepthBuffer, NULL, &m_inputLayout, device);
	ShaderSwapper::CompileShader("VS_Skybox.hlsl", "PS_Skybox.hlsl", &m_pixelShaderSkybox, &m_vertexShaderSkybox, &m_inputLayout, device);
	ShaderSwapper::CompileShader("", "PS_PrecomputeIBL.hlsl", &m_pixelShaderDiffuseIBL, NULL, &m_inputLayout, device, "", "DiffusePrecompute");
	ShaderSwapper::CompileShader("", "PS_PrecomputeIBL.hlsl", &m_pixelShaderSpecularIBL, NULL, &m_inputLayout, device, "", "SpecularPrecompute");
	ShaderSwapper::CompileShader("", "PS_PrecomputeIBL.hlsl", &m_pixelShaderEnvironmentBRDF, NULL, &m_inputLayout, device, "", "PrecomputeEnvironmentLUT");
	ShaderSwapper::CompileShader("", "PS_Unlit.hlsl", &m_pixelShaderUnlit, NULL, &m_inputLayout, device);
	ShaderSwapper::CompileShader("", "PS_BlinnPhong.hlsl", &m_pixelShaderBlinnPhong, NULL, &m_inputLayout, device);
	
	//Prepare SSAO data
	m_ssao = new ShaderSSAO(device);
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
	m_skyboxModel->LoadModel("cube.obj", device);
	CreateSkyboxCubemap();
}

void Renderer::CreateDeviceDependentResources()
{
	CreateConstantBuffers();
	PrepareAreaLightStructures();

	BaseLight::BaseLightStruct areaLights;
	areaLights.radius = 1.0f;
	areaLights.position = XMFLOAT3{ 50.0f, 40.0f, -35.0f };
	areaLights.area = areaLights.radius * areaLights.radius * 4.0f * 3.14f;
	areaLights.padding = XMFLOAT3{ 0,0,0 };

	//Sphere color
	areaLights.color = XMFLOAT3{ 1,1,1 };
	const float intensity = 100000.0f;
	const XMVECTOR multipliedColorIntensity = XMVectorMultiply(XMVECTOR{ areaLights.color.x, areaLights.color.y, areaLights.color.z }, XMVECTOR{ intensity, intensity, intensity });
	areaLights.color = XMFLOAT3{ multipliedColorIntensity.m128_f32[0], multipliedColorIntensity.m128_f32[1], multipliedColorIntensity.m128_f32[2] };
	
	areaLights.color.x /= areaLights.area * 3.14f;
	areaLights.color.y /= areaLights.area * 3.14f;
	areaLights.color.z /= areaLights.area * 3.14f;

	areaLights.type = static_cast<int>(BaseLight::LightType::Area);
	UpdateAreaLights({ areaLights }, 1);
}

void Renderer::CreateWindowSizeDependentResources()
{
	CreateViewAndPerspective();
}

void Renderer::Update()
{
	if (m_frameCount == MAXUINT) m_frameCount = 0;
	if (m_frameInfoBufferData.currentFrameCount < 0) {
		m_frameInfoBufferData.currentFrameCount = 0;
	}
	m_frameInfoBufferData.currentFrameCount++;
}

void Renderer::Render()
{
	m_profiler->StartFrame();
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();
	ID3D11RenderTargetView* renderTarget = m_deviceManager->GetRenderTarget();
	ID3D11DepthStencilView* depthStencil = m_deviceManager->GetDepthStencil();

	if (m_baseVertexShader && m_pixelShaderBunny && m_pixelShaderUnlit)
	{
		//Input layout is the same for all vertex shaders for now
		context->IASetInputLayout(m_inputLayout);
		m_deviceManager->SetBackBufferRenderTarget();

		RenderShadowMap();

		m_deviceManager->SetBackBufferRenderTarget();

		//m_renderTexture->SetAsActiveTarget(context, depthStencil, true, true);

		//Create world matrix
		m_constantBufferData.world = XMMatrixIdentity();
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(m_sphereModel->m_scale, m_sphereModel->m_scale, m_sphereModel->m_scale));
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixTranslation(0.0f, 0.0f, 0.0f));
		m_constantBufferData.world = XMMatrixTranspose(m_constantBufferData.world);

		//RenderGBuffer(Renderer::GBufferType::Position);
		//RenderGBuffer(Renderer::GBufferType::Normal);
		//RenderGBuffer(Renderer::GBufferType::Depth);

		//m_constantBufferData.world = XMMatrixIdentity();
		//RenderSSAO();
		//RenderToBackBuffer(m_normalBufferTexture);
		//return;

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

		//context->VSSetShader(m_baseVertexShader, NULL, 0);
		//context->PSSetShader(m_pixelShaderBunny, NULL, 0);

		context->VSSetShader(m_baseVertexShader, NULL, 0);
		context->PSSetShader(m_pixelShaderUnlit, NULL, 0);

		if (m_roughnessResourceView) context->PSSetShaderResources(1, 1, &m_roughnessResourceView);
		if (m_normalResourceView) context->PSSetShaderResources(2, 1, &m_normalResourceView);
		if (m_metallicResourceView) context->PSSetShaderResources(3, 1, &m_metallicResourceView);
		if (m_skyboxResourceView) context->PSSetShaderResources(4, 1, &m_skyboxResourceView);
		if (m_diffuseIBLResourceView) context->PSSetShaderResources(5, 1, &m_diffuseIBLResourceView);
		if (m_specularIBLResourceView) context->PSSetShaderResources(6, 1, &m_specularIBLResourceView);
		if (m_environmentBRDF && m_environmentBRDF->GetResourceView()) {
			auto tex = m_environmentBRDF->GetResourceView();
			context->PSSetShaderResources(7, 1, &tex);
		}
		if (m_areaLightSRV) context->PSSetShaderResources(13, 1, &m_areaLightSRV);

		//Render area lights shape - for debug purposes
		//for (const auto& light : m_areaLights)
		//{
		//	m_uberBufferData.unlitColor = XMFLOAT4{ light.color.x, light.color.y, light.color.z, 1.0f };
		//	MapResourceData();
		//	SetConstantBuffers();
		//	const float scale = m_sphereModel->m_scale * light.radius * 0.5f;

		//	m_constantBufferData.world = XMMatrixIdentity();
		//	m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(scale, scale, scale));
		//	m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixTranslation(light.position.x, light.position.y, light.position.z));
		//	m_constantBufferData.world = XMMatrixTranspose(m_constantBufferData.world);
		//	MapConstantBuffer();

		//	m_indexCount = m_sphereModel->Render(context);
		//	context->DrawIndexed(m_indexCount, 0, 0);
		//}

		/* Render plane */
		context->PSSetShader(m_pixelShaderBlinnPhong, NULL, 0);
		if (m_shadowMapTexture) { 
			auto view = m_shadowMapTexture->GetResourceView();
			context->PSSetShaderResources(0, 1, &view); 
		}

		m_constantBufferData.world = XMMatrixIdentity();
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(1, 1, 1));
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixRotationRollPitchYaw(m_groundPlaneModel->m_rotation.x, m_groundPlaneModel->m_rotation.y, m_groundPlaneModel->m_rotation.z));
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixTranslation(m_groundPlaneModel->m_position.x, m_groundPlaneModel->m_position.y, m_groundPlaneModel->m_position.z));
		m_constantBufferData.world = XMMatrixTranspose(m_constantBufferData.world);

		MapConstantBuffer();

		m_uberBufferData.unlitColor = XMFLOAT4{ 0, 0, 0, 1.0f };
		MapResourceData();
		SetConstantBuffers();

		m_indexCount = m_groundPlaneModel->Render(context);
		context->DrawIndexed(m_indexCount, 0, 0);
		/* End render plane */

		context->VSSetShader(m_baseVertexShader, NULL, 0);
		//context->PSSetShader(m_pixelShaderBunny, NULL, 0);
		m_indexCount = m_sphereModel->Render(context);

		RenderSphereFromGrid(XMFLOAT3{ m_groundPlaneModel->m_position.x, m_groundPlaneModel->m_position.y + 1.0f, m_groundPlaneModel->m_position.z }, 1.0f, 0.0f);

		//m_profiler->StartProfiling("Main render loop");
		//constexpr int columnCount = 6;
		//constexpr int rowCount = 6;
		//for (int x = 0; x < columnCount; ++x)
		//{
		//	for (int y = 0; y < rowCount; ++y)
		//	{
		//		const float roughness = max(static_cast<float>(x) * (1.0f / max(1.0f, (columnCount - 1))), 0.001f);
		//		const float metallic = static_cast<float>(y) * (1.0f / max(1, (rowCount - 1)));
		//		RenderSphereFromGrid(XMFLOAT3{ x * 10.0f, y * 10.0f, 0.0f}, roughness, metallic);
		//	}
		//}
		//m_profiler->EndProfiling("Main render loop");

		DrawSkybox();
		//RenderToBackBuffer(m_renderTexture);
		static int counterToStartAction = 1;
		counterToStartAction--;
		if (counterToStartAction == 0)
		{
			ConvoluteDiffuseSkybox();
			ConvoluteSpecularSkybox();
			PrecomputeEnvironmentBRDF();

			m_deviceManager->SetBackBufferRenderTarget();
		}

		if (DO_SCREENSHOT_NEXT_FRAME)
		{
			DO_SCREENSHOT_NEXT_FRAME = false;
			std::string filename = "Screenshots/ss_" + Framework::currentDateTime() + ".png";
			std::wstring wFilename{ filename.begin(), filename.end() };
			SaveTextureToFile(m_renderTexture, wFilename.c_str());
		}
	}
	m_profiler->EndFrame();
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
	if (x != 0 || y != 0 || z != 0)
	{
		m_profiler->ResetProfiling();
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
	if (x != 0 || y != 0 || z != 0)
	{
		m_profiler->ResetProfiling();
	}
}

void Renderer::AddCameraRotation(XMFLOAT3 addRot)
{
	AddCameraPosition(addRot.x, addRot.y, addRot.z);
}

HRESULT Renderer::CreateConstantBuffers()
{
	HRESULT result = S_OK;
	ID3D11Device* device = m_deviceManager->GetDevice();

	//Create matrix buffer
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.ByteWidth = 0;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;

	//Continue creating constant buffers
	constantBufferDesc.ByteWidth = sizeof(ConstantBufferStruct);
	result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffer);
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

	constantBufferDesc.ByteWidth = sizeof(SpecialBufferPrecomputeIBLStruct);
	result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_specialBufferPrecomputeIBL);
	if (FAILED(result))
	{
		return result;
	}

	constantBufferDesc.ByteWidth = sizeof(FrameInfoBufferStruct);
	result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_frameInfoBuffer);
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

void Renderer::PrepareAreaLightStructures()
{
	auto device = m_deviceManager->GetDevice();

	//Create structured buffer for area lights
	D3D11_BUFFER_DESC areaLightBufferDesc;
	areaLightBufferDesc.ByteWidth = sizeof(BaseLight::BaseLightStruct) * m_areaLightCount;
	areaLightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	areaLightBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	areaLightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	areaLightBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	areaLightBufferDesc.StructureByteStride = sizeof(BaseLight::BaseLightStruct);

	assert(SUCCEEDED(device->CreateBuffer(&areaLightBufferDesc, nullptr, &m_areaLightBuffer)));

	//Prepare area light SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvAreaLightDesc;
	srvAreaLightDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvAreaLightDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvAreaLightDesc.Buffer.ElementOffset = 0;
	srvAreaLightDesc.Buffer.ElementWidth = m_areaLightCount;

	assert(SUCCEEDED(device->CreateShaderResourceView(m_areaLightBuffer, &srvAreaLightDesc, &m_areaLightSRV)));
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
	MapConstantBuffer();

	//MAP ADDITIONAL DATA
	if (m_uberBuffer)
	{
		const HRESULT result = context->Map(m_uberBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		UberBufferStruct* dataPtr = static_cast<UberBufferStruct*>(mappedResource.pData);

		dataPtr->unlitColor = m_uberBufferData.unlitColor;
		dataPtr->viewerPosition = m_uberBufferData.viewerPosition;
		dataPtr->directionalLightDirection = m_uberBufferData.directionalLightDirection;
		dataPtr->directionalLightColor = m_uberBufferData.directionalLightColor;

		//XMMATRIX lightView = CreateViewMatrix(XMVECTOR{ 0,0,0 }, XMFLOAT3{ m_uberBufferData.directionalLightDirection.x, m_uberBufferData.directionalLightDirection.y, m_uberBufferData.directionalLightDirection.z });
		//XMMATRIX lightProj = CreateOrthographicMatrix();

		float radius = m_groundPlaneModel->GetBounds().GetRadius();
		XMFLOAT3 dir = m_uberBufferData.directionalLightDirection;
		XMVECTOR lightPos = XMVECTOR{ -2.0f * radius * dir.x, -2.0f * radius * dir.y, -2.0f * radius * dir.z };
		XMFLOAT3 target = m_groundPlaneModel->GetBounds().GetCenter();
		XMVECTOR up = { 0, 1, 0 };

		//XMMATRIX lightView = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(lightPos, XMVECTOR{ target.x, target.y, target.z }, up));

		auto sphereCenterLS = XMVector3TransformCoord(XMVECTOR{ target.x, target.y, target.z }, m_constantBufferData.view);
		auto l = sphereCenterLS.m128_f32[0] - radius;
		auto b = sphereCenterLS.m128_f32[1] - radius;
		auto n = sphereCenterLS.m128_f32[2] - radius;
		auto r = sphereCenterLS.m128_f32[0] + radius;
		auto t = sphereCenterLS.m128_f32[1] + radius;
		auto f = sphereCenterLS.m128_f32[2] + radius;
		//XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

		XMMATRIX lightView = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(XMVECTOR{ 15.0f, 50.0f, -85.0f }, XMVECTOR{ 0, 0, 0 }, up));
		constexpr float FOV = 3.14f / 2.0f;
		constexpr auto screenAspect = 1.0f;
		XMMATRIX lightProj = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(FOV, screenAspect, 0.01f, 100.0f));

		dataPtr->lightViewMatrix = lightView;
		dataPtr->lightProjMatrix = lightProj;
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

	//MAP FRAME INFO DATA
	if (m_frameInfoBuffer)
	{
		const HRESULT result = context->Map(m_frameInfoBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
			return;

		FrameInfoBufferStruct* dataPtr = static_cast<FrameInfoBufferStruct*>(mappedResource.pData);

		dataPtr->currentFrameCount = m_frameInfoBufferData.currentFrameCount;
		dataPtr->renderTargetSize = XMFLOAT2{ 1280,720 }; //TODO magical number
		dataPtr->padding = 0;
		context->Unmap(m_frameInfoBuffer, 0);
	}
}

void Renderer::SetConstantBuffers()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	if (m_constantBuffer) context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
	if (m_uberBuffer) context->VSSetConstantBuffers(7, 1, &m_uberBuffer);
	if (m_uberBuffer) context->PSSetConstantBuffers(7, 1, &m_uberBuffer);
	if (m_propertyBuffer) context->PSSetConstantBuffers(0, 1, &m_propertyBuffer);

	if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
	if (m_baseResourceView) context->PSSetShaderResources(0, 1, &m_baseResourceView);
}

void Renderer::MapConstantBuffer()
{
	if (m_constantBuffer)
	{
		ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		assert(SUCCEEDED(context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)));

		ConstantBufferStruct* dataPtr = static_cast<ConstantBufferStruct*>(mappedResource.pData);

		dataPtr->world = m_constantBufferData.world;
		dataPtr->view = m_constantBufferData.view;
		dataPtr->projection = m_constantBufferData.projection;
		context->Unmap(m_constantBuffer, 0);
	}
}

void Renderer::UpdateAreaLights(std::vector<BaseLight::BaseLightStruct> data, int lightCount)
{
	if (lightCount != m_areaLightCount)
	{
		//System is not prepared to change area light count
		assert(false);
	}
	m_areaLights = data;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	assert(SUCCEEDED(context->Map(m_areaLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)));

	BaseLight::BaseLightStruct* dataPtr = static_cast<BaseLight::BaseLightStruct*>(mappedResource.pData);

	dataPtr->color = m_areaLights.at(0).color;
	dataPtr->position = m_areaLights.at(0).position;
	dataPtr->radius = m_areaLights.at(0).radius;
	dataPtr->type = m_areaLights.at(0).type;

	//memcpy(mappedResource.pData, &data, sizeof(BaseLight::BaseLightStruct) * m_areaLightCount);
	context->Unmap(m_areaLightBuffer, 0);
}

void Renderer::DrawSkybox()
{
	if (m_skyboxModel && m_pixelShaderSkybox && m_vertexShaderSkybox)
	{
		ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

		m_deviceManager->UseSkyboxDepthStencilStateAndRasterizer();

		m_skyboxModel->m_position = XMFLOAT3{ m_cameraPosition.x - 0.5f, m_cameraPosition.y - 0.5f, m_cameraPosition.z - 0.5f };
		m_constantBufferData.world = XMMatrixIdentity();
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixTranslation(m_skyboxModel->m_position.x, m_skyboxModel->m_position.y, m_skyboxModel->m_position.z));
		m_constantBufferData.world = XMMatrixTranspose(m_constantBufferData.world);
		MapResourceData();

		if (m_constantBuffer) context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
		if (m_uberBuffer) context->PSSetConstantBuffers(7, 1, &m_uberBuffer);

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

bool Renderer::CreateSkyboxCubemap()
{
	std::array<std::wstring, 6> filenames{ L"Resources/Skyboxes/posx.jpg", L"Resources/Skyboxes/negx.jpg", L"Resources/Skyboxes/posy.jpg", L"Resources/Skyboxes/negy.jpg", L"Resources/Skyboxes/posz.jpg", L"Resources/Skyboxes/negz.jpg" };
	//std::array<std::wstring, 6> filenames{ L"Resources/Skyboxes/Trees/posx.bmp", L"Resources/Skyboxes/Trees/negx.bmp", L"Resources/Skyboxes/Trees/posy.bmp", L"Resources/Skyboxes/Trees/negy.bmp", L"Resources/Skyboxes/Trees/posz.bmp", L"Resources/Skyboxes/Trees/negz.bmp" };
	return ConstructCubemap(filenames, &m_skyboxResourceView);
}

bool Renderer::ConvoluteDiffuseSkybox()
{
	std::array<std::wstring, 6> filenames{ L"Resources/Skyboxes/ibl_posx.png", L"Resources/Skyboxes/ibl_negx.png", L"Resources/Skyboxes/ibl_posy.png", L"Resources/Skyboxes/ibl_negy.png", L"Resources/Skyboxes/ibl_posz.png", L"Resources/Skyboxes/ibl_negz.png" };

	for (int i = 0; i < 6; ++i)
	{
		ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();
		m_diffuseConvolutionTexture->SetAsActiveTarget(context, NULL, true, false, XMFLOAT4{ 0,0,0,0 });
		//m_deviceManager->SetBackBufferRenderTarget();
		m_indexCount = m_backBufferQuadModel->Render(context);

		context->VSSetShader(m_vertexShaderBackBuffer, NULL, 0);
		context->PSSetShader(m_pixelShaderDiffuseIBL, NULL, 0);

		if (m_constantBuffer)
		{
			MapConstantBuffer();
			context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
		}

		if (m_specialBufferPrecomputeIBL)
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			const HRESULT result = context->Map(m_specialBufferPrecomputeIBL, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			assert(SUCCEEDED(result));
			SpecialBufferPrecomputeIBLStruct* dataPtr = static_cast<SpecialBufferPrecomputeIBLStruct*>(mappedResource.pData);

			dataPtr->cubemapFaceIndex = i;
			dataPtr->roughness = 0;
			dataPtr->padding = XMFLOAT2{};
			context->Unmap(m_specialBufferPrecomputeIBL, 0);

			context->PSSetConstantBuffers(13, 1, &m_specialBufferPrecomputeIBL);
		}
		if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
		if (m_skyboxResourceView) context->PSSetShaderResources(0, 1, &m_skyboxResourceView);

		context->DrawIndexed(m_indexCount, 0, 0);
		SaveTextureToFile(m_diffuseConvolutionTexture, filenames[i].c_str());
	}

	ConstructCubemap(filenames, &m_diffuseIBLResourceView);
	return true;
}

void Renderer::ConvoluteSpecularSkybox()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	context->VSSetShader(m_vertexShaderBackBuffer, NULL, 0);
	context->PSSetShader(m_pixelShaderSpecularIBL, NULL, 0);

	m_indexCount = m_backBufferQuadModel->Render(context);

	static const std::array<std::wstring, 6> filenames{ L"Resources/Skyboxes/ibl_s_posx", L"Resources/Skyboxes/ibl_s_negx", L"Resources/Skyboxes/ibl_s_posy", L"Resources/Skyboxes/ibl_s_negy", L"Resources/Skyboxes/ibl_s_posz", L"Resources/Skyboxes/ibl_s_negz" };
	for (int i = 0; i < 6; ++i)
	{	
		for (int roughnessIndex = 0; roughnessIndex < SPECULAR_CONVOLUTION_MIPS; roughnessIndex++)
		{
			m_specularConvolutionTexture.at(roughnessIndex + i * SPECULAR_CONVOLUTION_MIPS)->SetViewportSize(static_cast<float>(256 >> roughnessIndex), static_cast<float>(256 >> roughnessIndex));
			m_specularConvolutionTexture.at(roughnessIndex + i * SPECULAR_CONVOLUTION_MIPS)->SetAsActiveTarget(context, NULL, true, false, XMFLOAT4{ 0,0,0,0 });
			if (m_constantBuffer)
			{
				MapConstantBuffer();
				context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
			}
			if (m_specialBufferPrecomputeIBL)
			{
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				const HRESULT result = context->Map(m_specialBufferPrecomputeIBL, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
				assert(SUCCEEDED(result));
				SpecialBufferPrecomputeIBLStruct* dataPtr = static_cast<SpecialBufferPrecomputeIBLStruct*>(mappedResource.pData);

				dataPtr->cubemapFaceIndex = i;
				dataPtr->roughness = static_cast<float>(roughnessIndex) / static_cast<float>(max(SPECULAR_CONVOLUTION_MIPS - 1, 1));
				dataPtr->padding = XMFLOAT2{};
				context->Unmap(m_specialBufferPrecomputeIBL, 0);

				context->PSSetConstantBuffers(13, 1, &m_specialBufferPrecomputeIBL);
			}
			if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
			if (m_skyboxResourceView) context->PSSetShaderResources(0, 1, &m_skyboxResourceView);

			context->DrawIndexed(m_indexCount, 0, 0);
			//SaveTextureToFile(m_specularConvolutionTexture, (filenames[i] + std::to_wstring(roughness) + L".png").c_str());
		}
	}

	if (SPECULAR_CONVOLUTION_MIPS > 0)
	{
		ConstructCubemapFromTextures(m_specularConvolutionTexture, &m_specularIBLResourceView);
	}
}

void Renderer::PrecomputeEnvironmentBRDF()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	context->VSSetShader(m_vertexShaderBackBuffer, NULL, 0);
	context->PSSetShader(m_pixelShaderEnvironmentBRDF, NULL, 0);

	m_indexCount = m_backBufferQuadModel->Render(context);
	m_environmentBRDF->SetAsActiveTarget(context, NULL, true, false, XMFLOAT4{ 0,0,0,0 });
	m_environmentBRDF->SetViewportSize(256,256);
	context->DrawIndexed(m_indexCount, 0, 0);

	//SaveTextureToFile(m_environmentBRDF, L"Resources/enviroBRDF.png");
}

bool Renderer::ConstructCubemap(std::array<std::wstring, 6> textureNames, ID3D11ShaderResourceView ** cubemapView)
{
	std::array<ID3D11Resource*, 6> textures{ nullptr };
	std::array<ID3D11ShaderResourceView*, 6> textureViews{ nullptr };

	for (size_t i = 0; i < 6; ++i)
	{
		if (FAILED(m_deviceManager->LoadTextureFromFile(textureNames[i].c_str(), &textures[i], &textureViews[i])))
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

	if (FAILED(m_deviceManager->GetDevice()->CreateShaderResourceView(texArray, &viewDesc, cubemapView)))
		return false;

	return true;
}

bool Renderer::ConstructCubemapFromTextures(std::array<RenderTexture*, SPECULAR_CONVOLUTION_MIPS * 6> textureFaces, ID3D11ShaderResourceView ** cubemapView)
{
	D3D11_TEXTURE2D_DESC importedTexDesc;
	((ID3D11Texture2D*)textureFaces[0]->GetResource())->GetDesc(&importedTexDesc);

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = importedTexDesc.Width;
	texDesc.Height = importedTexDesc.Height;
	texDesc.MipLevels = SPECULAR_CONVOLUTION_MIPS;
	texDesc.ArraySize = 6 * SPECULAR_CONVOLUTION_MIPS;
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

			context->CopySubresourceRegion(texArray, D3D11CalcSubresource(mipLevel, i, texDesc.MipLevels), 0, 0, 0, textureFaces[mipLevel + i * SPECULAR_CONVOLUTION_MIPS]->GetResource(), 0, &sourceRegion);
		}
	}

	// Create a resource view to the texture array.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	viewDesc.TextureCube.MostDetailedMip = 0;
	viewDesc.TextureCube.MipLevels = texDesc.MipLevels;

	if (FAILED(m_deviceManager->GetDevice()->CreateShaderResourceView(texArray, &viewDesc, cubemapView)))
		return false;

	return true;
}

bool Renderer::CreateDiffuseCubemapIBL()
{
	return false;
}

void Renderer::RenderToBackBuffer(RenderTexture * texture)
{
	if (texture)
	{
		if (auto texView = texture->GetResourceView()) {
			RenderToBackBuffer(texView);
		}
	}
}

void Renderer::RenderToBackBuffer(ID3D11ShaderResourceView * resource)
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	m_deviceManager->SetBackBufferRenderTarget(false, false);
	m_indexCount = m_backBufferQuadModel->Render(context);

	context->VSSetShader(m_vertexShaderBackBuffer, NULL, 0);
	context->PSSetShader(m_pixelShaderBackBuffer, NULL, 0);

	if (m_baseSamplerState) context->PSSetSamplers(0, 1, &m_baseSamplerState);
	if (resource) {
		context->PSSetShaderResources(0, 1, &resource);
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
	m_indexCount = m_sphereModel->Render(context);
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
		MapConstantBuffer();
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

void Renderer::RenderSphereFromGrid(XMFLOAT3 position, float roughness, float metallic)
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	m_constantBufferData.world = XMMatrixIdentity();
	m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(m_sphereModel->m_scale, m_sphereModel->m_scale, m_sphereModel->m_scale));
	m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixTranslation(position.x, position.y, position.z));
	m_constantBufferData.world = XMMatrixTranspose(m_constantBufferData.world);
	MapConstantBuffer();

	if (m_areaLightSRV)
	{
		UpdateAreaLights(m_areaLights, 1);
		context->PSSetShaderResources(13, 1, &m_areaLightSRV);
	}
	if (m_frameInfoBuffer) context->PSSetConstantBuffers(1, 1, &m_frameInfoBuffer);
	if (m_constantBuffer) context->PSSetConstantBuffers(0, 1, &m_constantBuffer);

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

		dataPtr->roughnessValue = roughness;
		dataPtr->metallicValue = metallic;
		//dataPtr->metallicValue = 1;
		dataPtr->f0 = min(max(m_specialBufferBRDFData.f0, 0.001f), 0.99999f);

		dataPtr->debugType = static_cast<int>(m_debugType);

		dataPtr->padding = 0;
		context->Unmap(m_specialBufferBRDF, 0);

		if (m_specialBufferBRDF) context->PSSetConstantBuffers(13, 1, &m_specialBufferBRDF);
	}

	context->DrawIndexed(m_indexCount, 0, 0);
}

void Renderer::RenderShadowMap()
{
	ID3D11DeviceContext* context = m_deviceManager->GetDeviceContext();

	m_shadowMapTexture->SetAsActiveTarget(m_deviceManager->GetDeviceContext(), m_deviceManager->GetDepthStencil(), true, true, XMFLOAT4{ 1,1,1,1 }, true);

	context->VSSetShader(m_baseVertexShader, NULL, 0);
	context->PSSetShader(NULL, NULL, 0);

	if (m_roughnessResourceView) context->PSSetShaderResources(1, 1, &m_roughnessResourceView);
	if (m_normalResourceView) context->PSSetShaderResources(2, 1, &m_normalResourceView);
	if (m_metallicResourceView) context->PSSetShaderResources(3, 1, &m_metallicResourceView);
	if (m_skyboxResourceView) context->PSSetShaderResources(4, 1, &m_skyboxResourceView);
	if (m_diffuseIBLResourceView) context->PSSetShaderResources(5, 1, &m_diffuseIBLResourceView);
	if (m_specularIBLResourceView) context->PSSetShaderResources(6, 1, &m_specularIBLResourceView);
	if (m_environmentBRDF && m_environmentBRDF->GetResourceView()) {
		auto tex = m_environmentBRDF->GetResourceView();
		context->PSSetShaderResources(7, 1, &tex);
	}
	if (m_areaLightSRV) context->PSSetShaderResources(13, 1, &m_areaLightSRV);

	/* Render plane */
	m_constantBufferData.world = XMMatrixIdentity();
	m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixScaling(1, 1, 1));
	m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixRotationRollPitchYaw(m_groundPlaneModel->m_rotation.x, m_groundPlaneModel->m_rotation.y, m_groundPlaneModel->m_rotation.z));
	m_constantBufferData.world = XMMatrixMultiply(m_constantBufferData.world, XMMatrixTranslation(m_groundPlaneModel->m_position.x, m_groundPlaneModel->m_position.y, m_groundPlaneModel->m_position.z));
	m_constantBufferData.world = XMMatrixTranspose(m_constantBufferData.world);
	
	//m_constantBufferData.view = CreateViewMatrix(XMVECTOR{ 0,0,0 }, XMFLOAT3{ m_uberBufferData.directionalLightDirection.x, m_uberBufferData.directionalLightDirection.y, m_uberBufferData.directionalLightDirection.z });
	float radius = m_groundPlaneModel->GetBounds().GetRadius();
	XMFLOAT3 dir = m_uberBufferData.directionalLightDirection;
	XMVECTOR lightPos = XMVECTOR{ -2.0f * radius * dir.x, -2.0f * radius * dir.y, -2.0f * radius * dir.z };
	XMFLOAT3 target = m_groundPlaneModel->GetBounds().GetCenter();
	XMVECTOR up = { 0, 1, 0 };

	//m_constantBufferData.view = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(lightPos, XMVECTOR{ target.x, target.y, target.z }, up));
	m_constantBufferData.view = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(XMVECTOR{ 15.0f, 50.0f, -85.0f }, XMVECTOR{ 0, 0, 0 }, up));

	auto sphereCenterLS = XMVector3TransformCoord(XMVECTOR{ target.x, target.y, target.z }, m_constantBufferData.view);
	auto l = sphereCenterLS.m128_f32[0] - radius;
	auto b = sphereCenterLS.m128_f32[1] - radius;
	auto n = sphereCenterLS.m128_f32[2] - radius;
	auto r = sphereCenterLS.m128_f32[0] + radius;
	auto t = sphereCenterLS.m128_f32[1] + radius;
	auto f = sphereCenterLS.m128_f32[2] + radius;

	//m_constantBufferData.projection = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
	//m_constantBufferData.projection = CreateOrthographicMatrix();
	constexpr float FOV = 3.14f / 2.0f;
	constexpr auto screenAspect = 1.0f;
	m_constantBufferData.projection = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(FOV, screenAspect, 0.01f, 100.0f));

	MapConstantBuffer();

	m_uberBufferData.unlitColor = XMFLOAT4{ 0, 0, 0, 1.0f };
	MapResourceData();
	SetConstantBuffers();

	m_indexCount = m_groundPlaneModel->Render(context);
	context->DrawIndexed(m_indexCount, 0, 0);

	m_indexCount = m_sphereModel->Render(context);
	RenderSphereFromGrid(XMFLOAT3{ m_groundPlaneModel->m_position.x, m_groundPlaneModel->m_position.y + 1.0f, m_groundPlaneModel->m_position.z }, 1.0f, 0.0f);

	CreateViewAndPerspective();
}

void Renderer::SaveTextureToFile(RenderTexture * texture, const wchar_t* name)
{
	if (texture && texture->GetResourceView() && m_deviceManager->GetDeviceContext())
	{
		ID3D11Resource* resource;
		texture->GetResourceView()->GetResource(&resource);
		const HRESULT result = SaveWICTextureToFile(m_deviceManager->GetDeviceContext(), resource, GUID_ContainerFormatPng, name);
		if (FAILED(result))	{
			assert(false);
		}
	}
}

XMMATRIX Renderer::CreateViewMatrix(const XMVECTOR lookAt, const XMFLOAT3 position)
{
	const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
	const DirectX::XMVECTOR eye = DirectX::XMVectorSet(position.x, position.y, position.z, 0.0f);

	//Create view matrix
	return DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(eye, lookAt, up));
}

XMMATRIX Renderer::CreateProjectionMatrix()
{
	constexpr float FOV = 3.14f / 4.0f;
	const float aspectRatio = m_deviceManager->GetAspectRatio();
	return DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(FOV, aspectRatio, 0.01f, 100.0f));
}

XMMATRIX Renderer::CreateOrthographicMatrix()
{
	return DirectX::XMMatrixTranspose(DirectX::XMMatrixOrthographicLH(1280, 720, 0.01f, 100.0f));
}
