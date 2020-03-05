#include "ShaderSwapper.h"
#include <d3dcompiler.h>
#include "ModelDX.h"

bool ShaderSwapper::CompileShader(std::string vertexShaderName, std::string pixelShaderName, ID3D11PixelShader ** pixelShader, ID3D11VertexShader ** vertexShader, ID3D11InputLayout ** inputLayout, ID3D11Device * device)
{
	////if (*vertexShader) {
	////	(*vertexShader)->Release();
	////}
	////if (*pixelShader) {
	////	(*pixelShader)->Release(); 
	////}

	//if (vertexShader && (*vertexShader))
	//{
	//	(*vertexShader)->Release();
	//}


	HRESULT result;
	ID3D10Blob* vertexShaderBuffer{ nullptr };
	ID3D10Blob* pixelShaderBuffer{ nullptr };
	ID3DBlob* errorBlob = nullptr;

	//Compile shaders	
	if (vertexShaderName != "" && vertexShader != NULL)
	{
		const std::string vertexShaderFullPath = "Shaders/" + vertexShaderName;
		const std::wstring vertexShaderFullPathW = std::wstring(vertexShaderFullPath.begin(), vertexShaderFullPath.end());
		result = D3DCompileFromFile(vertexShaderFullPathW.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vertexShaderBuffer, &errorBlob);
		if (FAILED(result))
		{
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}
			return result;
		}

		//Fill shaders data
		result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, vertexShader);
		if (FAILED(result))
		{
			return result;
		}

		//Create polygon layout
		const auto polygonLayout = ShaderSwapper::GetStandardPolygonLayout();
		result = device->CreateInputLayout(polygonLayout.data(), static_cast<UINT>(polygonLayout.size()), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), inputLayout);
		if (FAILED(result))
		{
			return result;
		}

		//Cleanup pointers
		vertexShaderBuffer->Release();
	}
	if (pixelShaderName != "" && pixelShader != NULL)
	{
		const std::string pixelShaderFullPath = "Shaders/" + pixelShaderName;
		const std::wstring pixelShaderFullPathW = std::wstring(pixelShaderFullPath.begin(), pixelShaderFullPath.end());
		result = D3DCompileFromFile(pixelShaderFullPathW.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pixelShaderBuffer, &errorBlob);
		if (FAILED(result))
		{
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}
			return result;
		}

		//Fill shaders data
		result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, pixelShader);
		if (FAILED(result))
		{
			return result;
		}

		pixelShaderBuffer->Release();
	}

	if (errorBlob)
	{
		errorBlob->Release();
	}
	return result;
}

std::array<D3D11_INPUT_ELEMENT_DESC, 3> ShaderSwapper::GetStandardPolygonLayout()
{
	std::array<D3D11_INPUT_ELEMENT_DESC, 3> polygonLayout;
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "NORMAL";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	polygonLayout[2].SemanticName = "TEXCOORD";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	return polygonLayout;
}