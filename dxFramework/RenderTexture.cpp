#include "RenderTexture.h"

RenderTexture::RenderTexture(int width, int height, ID3D11Device * device, DXGI_FORMAT format /* = DXGI_FORMAT_B8G8R8A8_UNORM */)
{
	assert(device);
	assert(width > 0 && height > 0);
	assert(InitializeRenderTexture(width, height, device, format));
}

void RenderTexture::SetAsActiveTarget(ID3D11DeviceContext * context, ID3D11DepthStencilView * depthStencilView, bool clearTarget /* = true */, bool clearDepth /* = false */, DirectX::XMFLOAT4 clearColor /* = { 0.098f, 0.439f, 0.439f, 1.000f } */)
{
	assert(context);
	assert(depthStencilView);
	context->OMSetRenderTargets(1, &m_renderTargetView, depthStencilView);

	if (clearTarget){
		const float teal[] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
		context->ClearRenderTargetView(m_renderTargetView, teal);
	}
	if (clearDepth){
		context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	context->RSSetViewports(1, &m_viewport);
}

ID3D11Resource * RenderTexture::GetResource()
{
	return static_cast<ID3D11Resource*>(m_texture2D);
}

bool RenderTexture::InitializeRenderTexture(int width, int height, ID3D11Device * device, DXGI_FORMAT format)
{
	HRESULT result;

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	result = device->CreateTexture2D(&textureDesc, NULL, &m_texture2D);
	if (FAILED(result))
		return false;

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	result = device->CreateRenderTargetView(m_texture2D, &renderTargetViewDesc, &m_renderTargetView);
	if (FAILED(result))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	result = device->CreateShaderResourceView(m_texture2D, &shaderResourceViewDesc, &m_shaderResourceView);
	if (FAILED(result))
		return false;

	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftY = 0;
	m_viewport.TopLeftX = 0;

	return true;
}
