// dxFramework.cpp : Application entry point

#include "framework.h"
#include "Main.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	HRESULT result = S_OK;

	std::shared_ptr<Main> main = std::shared_ptr<Main>(new Main());
	result = main->CreateDesktopWindow();

	if (SUCCEEDED(result))
	{
		//Create device manager
		std::shared_ptr<DeviceManager> deviceManager = std::shared_ptr<DeviceManager>(new DeviceManager());
		deviceManager->CreateDeviceResources(main->GetWindowHandle());

		//Create renderer
		std::shared_ptr<Renderer> renderer = std::shared_ptr<Renderer>(new Renderer(deviceManager));
		renderer->CreateDeviceDependentResources();

		//Initialize device manager and renderer
		deviceManager->CreateWindowResources(main->GetWindowHandle());
		renderer->CreateWindowSizeDependentResources();

		//Start full screen?
		//deviceManager->SetFullscreenMode();

		//Fix after setting full screen

		//Run main loop
		main->Run(deviceManager, renderer);
	}

	return result;
}