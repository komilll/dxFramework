#pragma once

#include "framework.h"

class DeviceManager
{
public:
	HRESULT CreateDeviceResources(HWND hwnd);
	HRESULT CreateWindowResources(HWND hwnd);

	HRESULT ConfigureBackBuffer();
	HRESULT ReleaseBackBuffer();
	HRESULT SetFullscreenMode();
	HRESULT SetWindowedMode();

	void ConfigureSamplerState(ID3D11SamplerState** samplerState, D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_WRAP, D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_COMPARISON_FUNC comparisonFunction = D3D11_COMPARISON_ALWAYS) const;
	void TextureChooseWindow(ID3D11Resource** texture, ID3D11ShaderResourceView** textureView) const;
	void UseViewport();
	void SetBackBufferRenderTarget();

	ID3D11Device* GetDevice() const { return m_device; };
	ID3D11DeviceContext* GetDeviceContext() const { return m_deviceContext; };
	ID3D11RenderTargetView* GetRenderTarget() const { return m_renderTarget; };
	ID3D11DepthStencilView* GetDepthStencil() const { return m_depthStencilView; };
	ID3D11Resource* GetBackBufferResource() const { return static_cast<ID3D11Resource*>(m_backBuffer); };

	float GetAspectRatio() const;

	void Present();

private:
	bool LoadTextureFromFile(wchar_t* wFilePath, ID3D11Resource** texture, ID3D11ShaderResourceView** textureView) const;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;
	IDXGISwapChain* m_swapChain;

	ID3D11Texture2D* m_backBuffer;
	ID3D11RenderTargetView* m_renderTarget;

	ID3D11Texture2D* m_depthStencil;
	ID3D11DepthStencilView* m_depthStencilView;
	ID3D11DepthStencilState* m_depthStencilState;
	
	ID3D11RasterizerState* m_rasterState;

	D3D_FEATURE_LEVEL m_featureLevel;
	D3D11_TEXTURE2D_DESC m_backBufferDesc;
	D3D11_VIEWPORT m_viewport;
};