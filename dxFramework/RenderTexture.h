#pragma once
#ifndef _RENDER_TEXTURE_H_
#include "framework.h"

class RenderTexture
{
public:
	RenderTexture(int width, int height, ID3D11Device* device = nullptr, bool depthOnly = false, DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM);
	bool SetAsActiveTarget(ID3D11DeviceContext * deviceContext, ID3D11DepthStencilView * depthStencilView, bool clearTarget = true, bool clearDepth = false, DirectX::XMFLOAT4 clearColor = { 0.098f, 0.439f, 0.439f, 1.000f }, bool depthOnly = false);
	ID3D11RenderTargetView* GetRenderTargetView() { return m_renderTargetView; };
	ID3D11Resource* GetResource();
	ID3D11ShaderResourceView* GetResourceView() { return m_shaderResourceView; };
	ID3D11DepthStencilView* GetDepthStencilView() { return m_depthStencilView; };
	void SetViewportSize(float width, float height);

private:
	bool InitializeRenderTexture(int width, int height, ID3D11Device* device, DXGI_FORMAT format);
	bool InitializeRenderTextureForDepth(int width, int height, ID3D11Device* device);

private:
	D3D11_VIEWPORT m_viewport;
	ID3D11Texture2D* m_texture2D					 = NULL;
	ID3D11ShaderResourceView* m_shaderResourceView	 = NULL;
	ID3D11RenderTargetView* m_renderTargetView	     = NULL;
	ID3D11DepthStencilView * m_depthStencilView		 = NULL;
};

#endif // !_RENDER_TEXTURE_H_
