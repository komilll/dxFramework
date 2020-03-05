#pragma once
#ifndef _GUI_MANAGER_H_
#define _GUI_MANAGER_H_

#include "framework.h"
#include "DeviceManager.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#define IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
#define IMGUI_IMPL_WIN32_DISABLE_LINKING_XINPUT
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include "Renderer.h"

class GuiManager
{
public:	
	GuiManager(std::shared_ptr<DeviceManager> deviceManager, std::shared_ptr<Renderer> renderer);

	void DrawImGUI();

private:
	std::shared_ptr<DeviceManager> m_deviceManager;
	std::shared_ptr<Renderer> m_renderer;
};

#endif // !_GUI_MANAGER_H_
