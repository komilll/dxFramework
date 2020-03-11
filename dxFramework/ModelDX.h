#pragma once

#include "framework.h"
#include <vector>
#include <array>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

using namespace DirectX;

class ModelDX
{
public:
	struct VertexBufferStruct {
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT3 binormal;
		XMFLOAT2 uv;
	};

	struct Mesh {
		unsigned int vertexCount;
		unsigned int indexCount;

		VertexBufferStruct* vertices;
		unsigned long* indices;

		Mesh() = default;
		Mesh(VertexBufferStruct _vertices[], unsigned long _indices[]) : vertices{ _vertices }, indices{ _indices }
		{

		}
	};

	void SetFullScreenRectangleModel(ID3D11Device* device);
	void LoadModel(std::string path, ID3D11Device* device);
	Mesh GetMesh(int index) const { return m_meshes.at(index); };
	std::vector<Mesh> GetMeshes() const { return m_meshes; };
	//Return indices to render count
	unsigned int Render(ID3D11DeviceContext* context);

private:
	void ProcessNode(aiNode* node, const aiScene* scene);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

	bool CreateRectangle(ID3D11Device* device, float left, float right, float top, float bottom);

	bool PrepareBuffers(ID3D11Device* device);

	template <std::size_t N, typename T>
	std::vector<T> GetFirstIndices()
	{
		std::vector<T> vec(N);
		for (size_t i = 0; i < N; ++i)
			vec[i] = static_cast<T>(i);
		return vec;
	}
	template <std::size_t N, typename T>
	std::array<T, N> GetFirstIndicesArray()
	{
		std::array<T> arr(N);
		for (size_t i = 0; i < N; ++i)
			arr[i] = static_cast<T>(i);
		return arr;
	}

//VARIABLES
public:
	float m_scale = 1.0f;
	XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 m_rotation{ 0.0f, 0.0f, 0.0f };

private:
	std::vector<Mesh> m_meshes;
	ID3D11Buffer* m_vertexBuffer	 = NULL;
	ID3D11Buffer* m_indexBuffer		 = NULL;
	unsigned int m_indexCount		 = 0;
};