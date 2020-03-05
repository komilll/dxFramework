#include "ModelDX.h"

#include <cstdio>
#include <ctime>
#include <iostream>

void ModelDX::SetFullScreenRectangleModel(ID3D11Device * device)
{
	assert(CreateRectangle(device, -1.0f, 1.0f, 1.0f, -1.0f));
	assert(PrepareBuffers(device));
}

void ModelDX::LoadModel(std::string path, ID3D11Device * device)
{
	Assimp::Importer importer;
	char result[MAX_PATH];
	const std::string filename = path;
	path = std::string(result, GetModuleFileNameA(NULL, result, MAX_PATH));
	const auto index = path.find_last_of('\\');
	path = path.substr(0, index + 1) + filename;

	const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
	
	assert(scene);
	ProcessNode(scene->mRootNode, scene);
	assert(PrepareBuffers(device));
}


unsigned int ModelDX::Render(ID3D11DeviceContext* context)
{
	constexpr UINT stride = sizeof(ModelDX::Vertex);
	constexpr UINT offset = 0;

	context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return GetMesh(0).indexCount;
}

void ModelDX::ProcessNode(aiNode * node, const aiScene * scene)
{
	for (unsigned int i = 0; i < node->mNumMeshes; ++i) 
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_meshes.push_back(ProcessMesh(mesh, scene));
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(node->mChildren[i], scene);
	}
}

ModelDX::Mesh ModelDX::ProcessMesh(aiMesh * mesh, const aiScene * scene)
{
	Mesh localMesh;
	localMesh.vertices = new Vertex[mesh->mNumVertices];

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex;
		XMFLOAT3 tmp;

		tmp.x = mesh->mVertices[i].x;
		tmp.y = mesh->mVertices[i].y;
		tmp.z = mesh->mVertices[i].z;
		vertex.position = tmp;

		if (mesh->mNormals){
			tmp.x = mesh->mNormals[i].x;
			tmp.y = mesh->mNormals[i].y;
			tmp.z = mesh->mNormals[i].z;
			vertex.normal = tmp;
		}

		if (mesh->HasTextureCoords(0)){
			vertex.uv.x = mesh->mTextureCoords[0][i].x;
			vertex.uv.y = mesh->mTextureCoords[0][i].y;
		}
		else{
			vertex.uv = XMFLOAT2{ 0,0 };
		}

		localMesh.vertices[i] = vertex;
	}

	unsigned int indicesCount = 0;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		indicesCount += face.mNumIndices;
	}
	localMesh.indices = new unsigned long[indicesCount];
	localMesh.indexCount = indicesCount;
	localMesh.vertexCount = mesh->mNumVertices;
	size_t currentIndexCounter = 0;

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			localMesh.indices[currentIndexCounter++] = static_cast<unsigned long>(face.mIndices[j]);
		}
	}

	return localMesh;
	//return Mesh{ vertices, indices };
}

bool ModelDX::CreateRectangle(ID3D11Device * device, float left, float right, float top, float bottom)
{
	Mesh localMesh;
	constexpr size_t verticesCount = 6;

	localMesh.vertices = new Vertex[verticesCount];
	localMesh.indices = new unsigned long[verticesCount];
	localMesh.vertexCount = verticesCount;
	localMesh.indexCount = verticesCount;
	for (size_t i = 0; i < verticesCount; ++i)
	{
		localMesh.indices[i] = static_cast<unsigned long>(i);
	}

	//First triangle
	localMesh.vertices[0].position = XMFLOAT3(left, top, 0.0f);  // Top left.	
	localMesh.vertices[1].position = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.
	localMesh.vertices[2].position = XMFLOAT3(left, bottom, 0.0f);  // Bottom left.
	//Second triangle
	localMesh.vertices[3].position = XMFLOAT3(left, top, 0.0f);  // Top left.
	localMesh.vertices[4].position = XMFLOAT3(right, top, 0.0f);  // Top right.
	localMesh.vertices[5].position = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.

	//Set UV values
	{
		//First triangle	
		localMesh.vertices[0].uv = XMFLOAT2(0.0, 0.0);  // Top left.	
		localMesh.vertices[1].uv = XMFLOAT2(1.0, 1.0);  // Bottom right.
		localMesh.vertices[2].uv = XMFLOAT2(0.0, 1.0);  // Bottom left.
		//Second triangle
		localMesh.vertices[3].uv = XMFLOAT2(0.0, 0.0);  // Top left.
		localMesh.vertices[4].uv = XMFLOAT2(1.0, 0.0);  // Top right.
		localMesh.vertices[5].uv = XMFLOAT2(1.0, 1.0);  // Bottom right.
	}

	m_meshes.clear();
	m_meshes.push_back(localMesh);

	return true;
}

bool ModelDX::PrepareBuffers(ID3D11Device* device)
{
	HRESULT result = S_OK;
	Mesh mesh = GetMesh(0);

	//Create vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(ModelDX::Vertex) * mesh.vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(D3D11_SUBRESOURCE_DATA));
	vertexBufferData.pSysMem = mesh.vertices;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_vertexBuffer);
	if (FAILED(result))
	{
		//Failed to create vertex buffer
		return false;
	}

	m_indexCount = mesh.indexCount;

	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexBufferData;
	ZeroMemory(&indexBufferData, sizeof(D3D11_SUBRESOURCE_DATA));
	indexBufferData.pSysMem = mesh.indices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_indexBuffer);
	if (FAILED(result))
	{
		//Failed to create index buffer
		return false;
	}
	return true;
}
