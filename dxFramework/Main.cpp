#include "Main.h"

HRESULT Main::CreateDesktopWindow()
{
	if (m_hInstance == NULL)
	{
		m_hInstance = static_cast<HINSTANCE>(GetModuleHandle(NULL));
	}

	HICON hIcon = NULL;
	WCHAR szExePath[MAX_PATH];
	GetModuleFileName(NULL, szExePath, MAX_PATH);

	if (hIcon == NULL)
	{
		hIcon = ExtractIcon(m_hInstance, szExePath, 0);
	}

	WNDCLASS wndClass;
	wndClass.style = CS_DBLCLKS;
	wndClass.lpfnWndProc = Main::StaticWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = m_hInstance;
	wndClass.hIcon = hIcon;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = m_windowClassName.c_str();

	if (!RegisterClass(&wndClass))
	{
		DWORD dwError = GetLastError();
		if (dwError != ERROR_CLASS_ALREADY_EXISTS)
		{
			return HRESULT_FROM_WIN32(dwError);
		}
	}

	int x = CW_USEDEFAULT;
	int y = CW_USEDEFAULT;

	m_hMenu = NULL;

	constexpr int defaultWidth = 1280;
	constexpr int defaultHeight = 720;
	SetRect(&m_rect, 0, 0, defaultWidth, defaultHeight);
	AdjustWindowRect(&m_rect, WS_OVERLAPPEDWINDOW, (m_hMenu ? true : false));

	m_hwnd = CreateWindow(m_windowClassName.c_str(), L"dxFramework", WS_OVERLAPPEDWINDOW, x, y, (m_rect.right - m_rect.left), (m_rect.bottom - m_rect.top), 0, m_hMenu, m_hInstance, 0);
	if (m_hwnd == NULL)
	{
		DWORD dwError = GetLastError();
		return HRESULT_FROM_WIN32(dwError);
	}
	
	return S_OK;
}


HRESULT Main::Run(std::shared_ptr<DeviceManager> deviceManager, std::shared_ptr<Renderer> renderer)
{
	HRESULT result = S_OK;

	ID3D11DeviceContext* context = deviceManager->GetDeviceContext();
	ID3D11Device* device = deviceManager->GetDevice();
	ID3D11RenderTargetView* renderTarget = deviceManager->GetRenderTarget();
	ID3D11DepthStencilView* depthStencil = deviceManager->GetDepthStencil();

	m_inputManager = std::shared_ptr<InputManager>(new InputManager());
	m_guiManager = std::shared_ptr<GuiManager>(new GuiManager(deviceManager, renderer));

	if (!IsWindowVisible(m_hwnd))
	{
		ShowWindow(m_hwnd, SW_SHOWDEFAULT);
		UpdateWindow(m_hwnd);
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(device, context);

	// Main loop
	bool bGotMsg;
	MSG msg;
	msg.message = WM_NULL;
	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
	while (msg.message != WM_QUIT)
	{
		bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);
		if (bGotMsg)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (m_inputManager->IsKeyDown(VK_ESCAPE))
			{
				//Leave loop and cleanup system
				break;
			}

			//Change camera position
			{
				constexpr float xStrengthPosition = 1.0f;
				constexpr float yStrengthPosition = 1.0f;
				constexpr float zStrengthPosition = 1.0f;
				const float x = xStrengthPosition * (m_inputManager->IsKeyDown(VK_A) ? -1.0f : (m_inputManager->IsKeyDown(VK_D) ? 1.0f : 0.0f));
				const float y = yStrengthPosition * (m_inputManager->IsKeyDown(VK_E) ? 1.0f : (m_inputManager->IsKeyDown(VK_Q) ? -1.0f : 0.0f));
				const float z = zStrengthPosition * (m_inputManager->IsKeyDown(VK_W) ? 1.0f : (m_inputManager->IsKeyDown(VK_S) ? -1.0f : 0.0f));
				renderer->AddCameraPosition(x, y, z);
			}
			//Change camera rotation
			{
				constexpr float xStrengthRotation = 1.0f;
				constexpr float yStrengthRotation = 1.0f;
				const float x = xStrengthRotation * (m_inputManager->IsKeyDown(VK_LEFT) ? -1.0f : (m_inputManager->IsKeyDown(VK_RIGHT) ? 1.0f : 0.0f));
				const float y = yStrengthRotation * (m_inputManager->IsKeyDown(VK_UP) ? -1.0f : (m_inputManager->IsKeyDown(VK_DOWN) ? 1.0f : 0.0f));
				renderer->AddCameraRotation(y, x, 0.0f);
			}

			renderer->Update();

			renderer->Render();

			m_guiManager->DrawImGUI();

			deviceManager->Present();
		}
	}
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return result;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT Main::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui::GetCurrentContext() == NULL)
	{
		ImGui::CreateContext();
	}

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
		case WM_KEYDOWN:
			m_inputManager->KeyDown(static_cast<unsigned int>(wParam));
			return 0;

		case WM_KEYUP:
			m_inputManager->KeyUp(static_cast<unsigned int>(wParam));
			return 0;

		case WM_CLOSE:
			if (GetMenu(hwnd)){
				DestroyMenu(GetMenu(hwnd));
			}
			DestroyWindow(hwnd);
			UnregisterClass(m_windowClassName.c_str(), m_hInstance);
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
