#pragma once
#ifndef _SHADER_SWAPPER_H_
#define _SHADER_SWAPPER_H_

#include "framework.h"
#include <array>

class ShaderSwapper 
{
public:
	static bool CompileShader(std::string vertexShaderName, std::string pixelShaderName, ID3D11PixelShader** pixelShader, ID3D11VertexShader** vertexShader, ID3D11InputLayout** inputLayout, ID3D11Device* device);

	static std::array<D3D11_INPUT_ELEMENT_DESC, 3> GetStandardPolygonLayout();
};

#endif // !_SHADER_SWAPPER_H_
