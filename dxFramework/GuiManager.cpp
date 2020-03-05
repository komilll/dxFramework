#include "GuiManager.h"
#include "ShaderSwapper.h"

GuiManager::GuiManager(std::shared_ptr<DeviceManager> deviceManager, std::shared_ptr<Renderer> renderer)
{
	m_deviceManager = deviceManager;
	m_renderer = renderer;
	assert(m_deviceManager);
	assert(m_renderer);
}

void GuiManager::DrawImGUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	#define previewVertexShader m_renderer->m_baseVertexShader
	#define previewPixelShader m_renderer->m_pixelShaderBackBuffer

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

		if (ImGui::Button("Select texture")) {
			m_deviceManager->TextureChooseWindow(&m_renderer->m_baseResource, &m_renderer->m_baseResourceView);
		}
	}
	ImGui::End();

	ImGui::Begin("Directional light");
	{
		ImGui::DragFloat3("Direction", &m_renderer->m_directionalLightBufferData.direction.x, 0.01f, -1.0f, 1.0f, "%.2f");
		ImGui::DragFloat("Intensity", &m_renderer->m_directionalLightBufferData.intensity, 0.01f, 0.0f, 1.0f, "%.2f");
		ImGui::ColorPicker3("Color", &m_renderer->m_propertyBufferData.directionalLightColor.x);

		ImGui::LabelText("", "Object properties");
		ImGui::DragFloat("Roughness", &m_renderer->m_propertyBufferData.roughness, 0.01f, 0.0f, 1.0f, "%.2f");
	}
	ImGui::End();

	// Rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

