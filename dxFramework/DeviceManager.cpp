#include "DeviceManager.h"
#include <shlobj.h> //Used for openning explorer window
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include <DirectXMath.h>
#include "SaveSession.h"
#include <sstream>

HRESULT DeviceManager::CreateDeviceResources(HWND hwnd)
{
	HRESULT result = S_OK;

	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;

	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	desc.Windowed = TRUE;
	desc.BufferCount = 2;
	desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	desc.OutputWindow = hwnd;

	result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, deviceFlags, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, &m_device, &m_featureLevel, &m_deviceContext);
	//result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, deviceFlags, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, &desc, &m_swapChain, &m_device, &m_featureLevel, &m_deviceContext);
	if (FAILED(result))
	{
		return result;
	}

	ID3D11InfoQueue* infoQueue{ nullptr };
	m_device->QueryInterface(IID_PPV_ARGS(&infoQueue));
	if (infoQueue)
	{
		infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->Release();
		infoQueue = nullptr;
	}

	return result;
}

HRESULT DeviceManager::CreateWindowResources(HWND hwnd)
{
	HRESULT result = S_OK;

	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	desc.Windowed = TRUE;
	desc.BufferCount = 2;
	desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	desc.OutputWindow = hwnd;

	IDXGIDevice * dxgiDevice = 0;
	result = m_device->QueryInterface(__uuidof(IDXGIDevice), (void **)& dxgiDevice);
	IDXGIAdapter * adapter = 0;
	result = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void **)& adapter);
	IDXGIFactory* factory = 0;
	result = adapter->GetParent(__uuidof(IDXGIFactory), (void **)& factory);

	if (SUCCEEDED(result))
	{
		result = factory->CreateSwapChain(m_device, &desc, &m_swapChain);
	}

	result = ConfigureBackBuffer();

	dxgiDevice->Release();
	adapter->Release();
	factory->Release();

	return result;
}

HRESULT DeviceManager::ConfigureBackBuffer()
{
	HRESULT result = S_OK;

	//Get back buffer and render target
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&m_backBuffer);
	result = m_device->CreateRenderTargetView(m_backBuffer, nullptr, &m_renderTarget);

	//Create back buffer desc
	m_backBuffer->GetDesc(&m_backBufferDesc);
	D3D11_TEXTURE2D_DESC depthBufferTextureDesc;
	ZeroMemory(&depthBufferTextureDesc, sizeof(depthBufferTextureDesc));
	depthBufferTextureDesc.Width = static_cast<UINT>(m_backBufferDesc.Width),
	depthBufferTextureDesc.Height = static_cast<UINT>(m_backBufferDesc.Height);
	depthBufferTextureDesc.MipLevels = 1;
	depthBufferTextureDesc.ArraySize = 1;
	depthBufferTextureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthBufferTextureDesc.SampleDesc.Count = 1;
	depthBufferTextureDesc.SampleDesc.Quality = 0;
	depthBufferTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferTextureDesc.CPUAccessFlags = 0;
	depthBufferTextureDesc.MiscFlags = 0;

	//Create depth stencil and depth stencil view
	result = m_device->CreateTexture2D(&depthBufferTextureDesc, nullptr, &m_depthStencil);
	if (FAILED(result))
		return result;

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.StencilWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
	if (FAILED(result))
	{
		return false;
	}

	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_skyboxDepthStencilState);
	if (FAILED(result))
	{
		return false;
	}

	GetDeviceContext()->OMSetDepthStencilState(m_depthStencilState, 1);

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	result = m_device->CreateDepthStencilView(m_depthStencil, &depthStencilViewDesc, &m_depthStencilView);
	if (FAILED(result))
		return result;

	//Create and set viewport
	ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
	m_viewport.Height = static_cast<float>(m_backBufferDesc.Height);
	m_viewport.Width = static_cast<float>(m_backBufferDesc.Width);
	m_viewport.MinDepth = 0;
	m_viewport.MaxDepth = 1;

	m_deviceContext->RSSetViewports(1, &m_viewport);

	//Create rasterizer state
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = true;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterState);
	if (FAILED(result))
	{
		return false;
	}
	rasterDesc.CullMode = D3D11_CULL_NONE;
	result = m_device->CreateRasterizerState(&rasterDesc, &m_skyboxRasterState);
	if (FAILED(result))
	{
		return false;
	}

	m_deviceContext->RSSetState(m_rasterState);

	return result;
}

HRESULT DeviceManager::ReleaseBackBuffer()
{
	HRESULT result = S_OK;

	m_renderTarget->Release();
	m_backBuffer->Release();
	m_depthStencilView->Release();
	m_depthStencil->Release();
	//Delete references
	m_deviceContext->Flush();

	return result;
}

HRESULT DeviceManager::SetFullscreenMode()
{
	HRESULT result = S_OK;

	result = m_swapChain->SetFullscreenState(TRUE, NULL);

	ReleaseBackBuffer();

	result = m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(result))
		return result;

	result = ConfigureBackBuffer();
	return result;
}

HRESULT DeviceManager::SetWindowedMode()
{
	HRESULT result = S_OK;

	result = m_swapChain->SetFullscreenState(FALSE, NULL);

	ReleaseBackBuffer();

	result = m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(result))
		return result;

	result = ConfigureBackBuffer();
	return result;
}

void DeviceManager::TextureChooseWindow(ID3D11Resource ** texture, ID3D11ShaderResourceView ** textureView, std::string* pathToSave /* = NULL */) const
{
	PWSTR pszFilePath;
	wchar_t* wFilePath = 0;
	std::wstringstream ss;
	IFileOpenDialog *pFileOpen;
	const COMDLG_FILTERSPEC ddsSpec = { L"DDS (DirectDraw Surface)", L"*.dds" };
	const COMDLG_FILTERSPEC pngSpec = { L"PNG", L"*.png" };
	const COMDLG_FILTERSPEC allSpec = { L"All files", L"*.*" };
	const COMDLG_FILTERSPEC rgSpec[] = { allSpec, ddsSpec, pngSpec };

	// Create the FileOpenDialog object.
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

	if (SUCCEEDED(hr))
		hr = pFileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);

	if (SUCCEEDED(hr))
	{
		// Show the Open dialog box.
		hr = pFileOpen->Show(NULL);

		// Get the file name from the dialog box.
		if (SUCCEEDED(hr))
		{
			IShellItem *pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				// Display the file name to the user.
				if (SUCCEEDED(hr))
				{
					wFilePath = pszFilePath;

					if (FAILED(LoadTextureFromFile(wFilePath, texture, textureView)))
					{
						assert(false);
					}
					if (pathToSave)
					{
						ss << pszFilePath;
						const std::wstring ws = ss.str();
						const std::string str(ws.begin(), ws.end());

						*pathToSave = str;
						//Used to get only filename
						//const std::size_t found = str.find_last_of("/\\");
						//str.substr(found + 1);

						//SaveSession::UpdateSavedData();
					}
					CoTaskMemFree(pszFilePath);
				}
				pItem->Release();
			}
		}
		pFileOpen->Release();
	}
}

void DeviceManager::UseViewport()
{
	m_deviceContext->RSSetViewports(1, &m_viewport);
}

void DeviceManager::UseSkyboxDepthStencilStateAndRasterizer()
{
	GetDeviceContext()->OMSetDepthStencilState(m_skyboxDepthStencilState, 1);
	m_deviceContext->RSSetState(m_skyboxRasterState);
}

void DeviceManager::UseStandardDepthStencilStateAndRasterizer()
{
	GetDeviceContext()->OMSetDepthStencilState(m_depthStencilState, 1);
	m_deviceContext->RSSetState(m_rasterState);
}

void DeviceManager::SetBackBufferRenderTarget(bool clearTarget, bool clearDepth)
{
	const float teal[] = { 0.098f, 0.439f, 0.439f, 1.000f };
	m_deviceContext->OMSetRenderTargets(1, &m_renderTarget, m_depthStencilView);
	if (clearTarget) m_deviceContext->ClearRenderTargetView(m_renderTarget, teal);
	if (clearDepth) m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	UseViewport();
}

 void DeviceManager::ConfigureSamplerState(ID3D11SamplerState** samplerState, D3D11_TEXTURE_ADDRESS_MODE addressMode, D3D11_FILTER filter, D3D11_COMPARISON_FUNC comparisonFunction) const
{
	D3D11_SAMPLER_DESC samplerDesc;

	samplerDesc.Filter = filter;
	samplerDesc.AddressU = addressMode;
	samplerDesc.AddressV = addressMode;
	samplerDesc.AddressW = addressMode;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = comparisonFunction;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	const HRESULT result = GetDevice()->CreateSamplerState(&samplerDesc, samplerState);
	if (FAILED(result))
	{
		assert(false);
	}
}

float DeviceManager::GetAspectRatio() const
{
	return static_cast<float>(m_backBufferDesc.Width) / static_cast<float>(m_backBufferDesc.Height);
}

void DeviceManager::Present()
{
	m_swapChain->Present(1, 0);
	m_deviceContext->ClearState();
}

HRESULT DeviceManager::LoadTextureFromFile(const wchar_t * wFilePath, ID3D11Resource ** texture, ID3D11ShaderResourceView ** textureView) const
{
	assert(GetDevice());
	assert(GetDeviceContext());
	if (*texture != nullptr) (*texture)->Release();
	if (*textureView != nullptr) (*textureView)->Release();

	HRESULT result = DirectX::CreateDDSTextureFromFile(GetDevice(), wFilePath, texture, textureView);
	if (FAILED(result))	{
		result = DirectX::CreateWICTextureFromFile(GetDevice(), GetDeviceContext(), wFilePath, texture, textureView);
	}

	if (FAILED(result))
	{
		if (*texture != nullptr) (*texture)->Release();
		if (*textureView != nullptr) (*textureView)->Release();
		texture = nullptr;
		textureView = nullptr;
	}
	return result;
}
