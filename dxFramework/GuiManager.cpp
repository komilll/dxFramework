#include "GuiManager.h"
#include "ShaderSwapper.h"

GuiManager::GuiManager(std::shared_ptr<DeviceManager> deviceManager, std::shared_ptr<Renderer> renderer, std::shared_ptr<SaveSession> saveSession)
{
	m_deviceManager = deviceManager;
	m_renderer = renderer;
	m_saveSession = saveSession;
	assert(m_deviceManager);
	assert(m_renderer);
	assert(m_saveSession);

	m_saveSession->TryToLoadTextures();
}

void GuiManager::DrawImGUI()
{
	constexpr std::array<const char* const, static_cast<unsigned int>(Renderer::NdfType::MAX)> ndfTypeNames{ "Beckmann", "GGX", "Blinn-Phong" };
	constexpr std::array<const char* const, static_cast<unsigned int>(Renderer::GeometryType::MAX)> geomTypeNames{ "Implicit", "Neumann", "Cook-Torrance", "Kelemen", "Smith", "Beckmann", "GGX", "Schlick-Beckmann", "Schlick-GGX" };
	constexpr std::array<const char* const, static_cast<unsigned int>(Renderer::FresnelType::MAX)> fresnelTypeNames{ "None", "Schlick", "CT" };

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	#define previewVertexShader m_renderer->m_baseVertexShader
	#define previewPixelShader m_renderer->m_pixelShaderBunny

	ImGui::Begin("Swap shader window");
	{
		if (ImGui::Button("Lambert")) {
			ShaderSwapper::CompileShader("VS_Base.hlsl", "PS_Lambert.hlsl", &previewPixelShader, &previewVertexShader, &m_renderer->m_inputLayout, m_deviceManager->GetDevice());
		}
		ImGui::SameLine();
		if (ImGui::Button("Blinn-Phong")) {
			ShaderSwapper::CompileShader("VS_Base.hlsl", "PS_BlinnPhong.hlsl", &previewPixelShader, &previewVertexShader, &m_renderer->m_inputLayout, m_deviceManager->GetDevice());
		}
		ImGui::SameLine();
		if (ImGui::Button("Oren-Nayar")) {
			ShaderSwapper::CompileShader("VS_Base.hlsl", "PS_OrenNayar.hlsl", &previewPixelShader, &previewVertexShader, &m_renderer->m_inputLayout, m_deviceManager->GetDevice());
		}

		if (ImGui::Button("Select ALBEDO")) {
			m_deviceManager->TextureChooseWindow(&m_renderer->m_baseResource, &m_renderer->m_baseResourceView, &m_saveSession->m_albedoTextureName);
			m_saveSession->UpdateSavedData();
		}
		if (ImGui::Button("Select NORMAL")) {
			m_deviceManager->TextureChooseWindow(&m_renderer->m_normalResource, &m_renderer->m_normalResourceView, &m_saveSession->m_normalTextureName);
			m_saveSession->UpdateSavedData();
		}
		if (ImGui::Button("Select ROUGHNESS")) {
			m_deviceManager->TextureChooseWindow(&m_renderer->m_roughnessResource, &m_renderer->m_roughnessResourceView, &m_saveSession->m_roughnessTextureName);
			m_saveSession->UpdateSavedData();
		}
		if (ImGui::Button("Select METALLIC")) {
			m_deviceManager->TextureChooseWindow(&m_renderer->m_metallicResource, &m_renderer->m_metallicResourceView, &m_saveSession->m_metallicTextureName);
			m_saveSession->UpdateSavedData();
		}
	}
	ImGui::End();

	ImGui::Begin("BRDF settings");
	{
		if (ImGui::CollapsingHeader("Normal distribution function", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (size_t i = 0; i < ndfTypeNames.size(); ++i)
			{
				if (ImGui::Selectable(ndfTypeNames[i], static_cast<int>(m_renderer->m_ndfType) == i))
				{
					m_renderer->m_ndfType = static_cast<Renderer::NdfType>(i);
				}
			}
		}
		if (ImGui::CollapsingHeader("Geometry shadowing function", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (size_t i = 0; i < geomTypeNames.size(); ++i)
			{
				if (ImGui::Selectable(geomTypeNames[i], static_cast<int>(m_renderer->m_geometryType) == i))
				{
					m_renderer->m_geometryType = static_cast<Renderer::GeometryType>(i);
				}
			}
		}
		if (ImGui::CollapsingHeader("Fresnel function", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (size_t i = 0; i < fresnelTypeNames.size(); ++i)
			{
				if (ImGui::Selectable(fresnelTypeNames[i], static_cast<int>(m_renderer->m_fresnelType) == i))
				{
					m_renderer->m_fresnelType = static_cast<Renderer::FresnelType>(i);
				}
			}
		}
		//ImGui::ListBox("Normal distribution function", reinterpret_cast<int*>(&m_renderer->m_ndfType), ndfTypeNames, static_cast<int>(Renderer::NdfType::MAX));
	}
	ImGui::End();

	ImGui::Begin("Settings");
	{
		ImGui::DragFloat3("Direction", &m_renderer->m_directionalLightBufferData.direction.x, 0.01f, -1.0f, 1.0f, "%.2f");
		ImGui::DragFloat("Intensity", &m_renderer->m_directionalLightBufferData.intensity, 0.01f, 0.0f, 1.0f, "%.2f");
		ImGui::ColorPicker3("Color", &m_renderer->m_propertyBufferData.directionalLightColor.x);

		ImGui::LabelText("", "Object properties");
		ImGui::DragFloat("Roughness", &m_renderer->m_propertyBufferData.roughness, 0.01f, 0.0f, 1.0f, "%.2f");
		ImGui::DragFloat("F0", &m_renderer->m_specialBufferBRDFData.f0, 0.001f, 0.0f, 1.0f, "%.3f");

		ImGui::DragInt("Sample count", &m_renderer->m_specialBufferSSAOData.sampleCount, 0.25f, 1, 64);
		ImGui::DragFloat("Kernel radius", &m_renderer->m_specialBufferSSAOData.kernelRadius, 0.05f, 0.01f, 50.0f, "%.2f");
	}
	ImGui::End();

	// Rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	static bool doOnce = true;
	if (doOnce) {
		doOnce = !TryToLoadTextures();
	}
}

bool GuiManager::TryToLoadTextures()
{
	bool success = true;
	success &= TryToLoadTexture(&m_saveSession->m_albedoTextureName, &m_renderer->m_baseResource, &m_renderer->m_baseResourceView);
	success &= TryToLoadTexture(&m_saveSession->m_roughnessTextureName, &m_renderer->m_roughnessResource, &m_renderer->m_roughnessResourceView);
	success &= TryToLoadTexture(&m_saveSession->m_normalTextureName, &m_renderer->m_normalResource, &m_renderer->m_normalResourceView);
	success &= TryToLoadTexture(&m_saveSession->m_metallicTextureName, &m_renderer->m_metallicResource, &m_renderer->m_metallicResourceView);

	m_saveSession->UpdateSavedData();

	return success;
}

bool GuiManager::TryToLoadTexture(std::string* path, ID3D11Resource ** resource, ID3D11ShaderResourceView ** resourceView)
{
	if (*resourceView || *path == "") {
		return true;
	}
	const std::wstring wPath = std::wstring{ path->begin(), path->end() };
	HRESULT result = m_deviceManager->LoadTextureFromFile(wPath.c_str(), resource, resourceView);
	if (FAILED(result))
	{
		if (result == E_NOINTERFACE)
		{
			return false;
		}
		*path = "";
		return true;
	}
	return true;
}
