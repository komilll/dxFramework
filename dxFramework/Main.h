#pragma once

#include "DirectXHelpers.h"
#include <string>

#include "DeviceManager.h"
#include "Renderer.h"
#include "InputManager.h"
#include "GuiManager.h"
#include "SaveSession.h"

class Main
{
public:
	HRESULT CreateDesktopWindow();

	HWND GetWindowHandle() const { return m_hwnd; };

	HRESULT Run(std::shared_ptr<DeviceManager> deviceManager, std::shared_ptr<Renderer> renderer);
	static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HMENU m_hMenu;
	RECT m_rect;
	HWND m_hwnd;	
	bool m_renderGUI = true;
};

static XMFLOAT3 m_cameraPositionChange;
static XMFLOAT3 m_cameraRotationChange;
static std::shared_ptr<InputManager> m_inputManager;
static std::shared_ptr<GuiManager> m_guiManager;
static std::shared_ptr<SaveSession> m_saveSession;
static HINSTANCE m_hInstance;
static std::wstring m_windowClassName = L"dxFramework";