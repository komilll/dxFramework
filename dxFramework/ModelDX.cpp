#include "ModelDX.h"

#include <cstdio>
#include <ctime>
#include <iostream>

void ModelDX::CreateLine(ID3D11Device * device, XMFLOAT3 start, XMFLOAT3 end)
{
	m_topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Mesh localMesh;
	constexpr size_t verticesCount = 2;

	localMesh.vertices = new VertexBufferStruct[verticesCount];
	localMesh.indices = new unsigned long[verticesCount];
	localMesh.vertexCount = verticesCount;
	localMesh.indexCount = verticesCount;

	for (size_t i = 0; i < verticesCount; ++i) {
		localMesh.indices[i] = static_cast<unsigned long>(i);
	}

	localMesh.vertices[0].position = start;
	localMesh.vertices[1].position = end;

	m_meshes.clear();
	m_meshes.push_back(localMesh);

	assert(PrepareBuffers(device));
}

void ModelDX::CreatePlane(ID3D11Device * device, XMFLOAT2 size)
{
	assert(CreateRectangle(device, -size.x / 2, size.x / 2, size.y / 2, -size.y / 2));
	assert(PrepareBuffers(device));
}

void ModelDX::CreateCube(ID3D11Device * device, XMFLOAT3 min, XMFLOAT3 max)
{
	Mesh localMesh;
	constexpr size_t verticesCount = 8;

	localMesh.vertices = new VertexBufferStruct[verticesCount];
	localMesh.indices = new unsigned long[verticesCount];
	localMesh.vertexCount = verticesCount;
	localMesh.indexCount = verticesCount;

	for (size_t i = 0; i < verticesCount; ++i) {
		localMesh.indices[i] = static_cast<unsigned long>(i);
	}

	//Near
	localMesh.vertices[0].position = XMFLOAT3{ min.x, min.y, min.z }; //Bottom left
	localMesh.vertices[1].position = XMFLOAT3{ max.x, min.y, min.z }; //Bottom right
	localMesh.vertices[2].position = XMFLOAT3{ min.x, max.y, min.z }; //Top left
	localMesh.vertices[3].position = XMFLOAT3{ max.x, max.y, min.z }; //Top right
	//Far
	localMesh.vertices[0].position = XMFLOAT3{ min.x, min.y, max.z }; //Bottom left
	localMesh.vertices[1].position = XMFLOAT3{ max.x, min.y, max.z }; //Bottom right
	localMesh.vertices[2].position = XMFLOAT3{ min.x, max.y, max.z }; //Top left
	localMesh.vertices[3].position = XMFLOAT3{ max.x, max.y, max.z }; //Top right

	//Indices


	m_meshes.clear();
	m_meshes.push_back(localMesh);

	assert(PrepareBuffers(device));
}

void ModelDX::SetFullScreenRectangleModel(ID3D11Device * device, float left, float right, float top, float bottom)
{
	assert(CreateRectangle(device, left, right, top, bottom));
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
	UINT stride[2] { sizeof(ModelDX::VertexBufferStruct), sizeof(ModelDX::InstanceType) };
	UINT offset[2] { 0, 0 };
	ID3D11Buffer* buffers[2] { m_vertexBuffer, m_instanceBuffer };

	context->IASetVertexBuffers(0, 2, buffers, stride, offset);
	context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(m_topology);

	if (m_meshes.size() > 0)
		return GetMesh(0).indexCount;
	else
		return 0;
}

ModelDX::Bounds ModelDX::GetBounds(int meshIndex)
{
	ModelDX::Mesh mesh = GetMesh(meshIndex);
	Bounds bounds{ FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (unsigned int i = 0; i < mesh.vertexCount; ++i)
	{
		XMFLOAT3 pos = mesh.vertices[i].position;

		if (pos.x > bounds.maxX) bounds.maxX = pos.x;
		if (pos.x < bounds.minX) bounds.minX = pos.x;

		if (pos.y > bounds.maxY) bounds.maxY = pos.y;
		if (pos.y < bounds.minY) bounds.minY = pos.y;

		if (pos.z > bounds.maxZ) bounds.maxZ = pos.z;
		if (pos.z < bounds.minZ) bounds.minZ = pos.z;
	}

	bounds.minX += m_position.x;
	bounds.maxX += m_position.x;
	bounds.minY += m_position.y;
	bounds.maxY += m_position.y;
	bounds.minZ += m_position.z;
	bounds.maxZ += m_position.z;

	return bounds;
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
	localMesh.vertices = new VertexBufferStruct[mesh->mNumVertices];

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		//POSITION
		VertexBufferStruct vertex;
		vertex.position = XMFLOAT3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		//NORMAL
		if (mesh->mNormals) {
			vertex.normal = XMFLOAT3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		}
		else {
			assert(false);
		}

		//UV
		if (mesh->HasTextureCoords(0)) {
			vertex.uv.x = mesh->mTextureCoords[0][i].x;
			vertex.uv.y = mesh->mTextureCoords[0][i].y;
		}
		else {
			vertex.uv = XMFLOAT2{ 0,0 };
		}

		//TANGENT and BINORMAL
		if (!mesh->mBitangents || !mesh->mTangents)
		{
			if (i >= 2 && (i - 2) % 3 == 0)
			{
				//https://learnopengl.com/Advanced-Lighting/Normal-Mapping
				const XMFLOAT3 pos1 = localMesh.vertices[i - 2].position;
				const XMFLOAT3 pos2 = localMesh.vertices[i - 1].position;
				const XMFLOAT3 pos3 = localMesh.vertices[i].position;
				const XMFLOAT3 edge1 = XMFLOAT3{ pos2.x - pos1.x, pos2.y - pos1.y, pos2.z - pos1.z };
				const XMFLOAT3 edge2 = XMFLOAT3{ pos3.x - pos1.x, pos3.y - pos1.y, pos3.z - pos1.z };

				const XMFLOAT2 uv1 = localMesh.vertices[i - 2].uv;
				const XMFLOAT2 uv2 = localMesh.vertices[i - 1].uv;
				const XMFLOAT2 uv3 = localMesh.vertices[i].uv;
				const XMFLOAT2 deltaUV1 = XMFLOAT2{ uv2.x - uv1.x, uv2.y - uv1.y };
				const XMFLOAT2 deltaUV2 = XMFLOAT2{ uv3.x - uv1.x, uv3.y - uv1.y };

				const float denominator = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

				XMVECTOR tangent = XMVectorSet(denominator * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
												denominator * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
												denominator * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z),
												0.0f);
				tangent = XMVector3Normalize(tangent);

				XMVECTOR binormal = XMVectorSet(denominator * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
												denominator * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
												denominator * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z),
												0.0f);
				binormal = XMVector3Normalize(binormal);

				localMesh.vertices->tangent = XMFLOAT3{ tangent.m128_f32[0], tangent.m128_f32[1], tangent.m128_f32[2] };
				localMesh.vertices->binormal = XMFLOAT3{ binormal.m128_f32[0], binormal.m128_f32[1], binormal.m128_f32[2] };
			}			
		}
		else
		{
			if (mesh->mBitangents) {
				vertex.binormal = XMFLOAT3{ mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
			}

			if (mesh->mTangents) {
				vertex.tangent = XMFLOAT3{ mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
			}
		}

		//Store in array
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

	localMesh.vertices = new VertexBufferStruct[verticesCount];
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

	localMesh.vertices[0].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[1].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[2].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[3].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[4].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[5].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);

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
	vertexBufferDesc.ByteWidth = sizeof(ModelDX::VertexBufferStruct) * mesh.vertexCount;
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

	InstanceType* instances;
	D3D11_BUFFER_DESC instanceBufferDesc;
	D3D11_SUBRESOURCE_DATA instanceData;

	m_instanceCount = 4;
	instances = new InstanceType[m_instanceCount];
	if (!instances) 
	{
		return false;
	}

	instances[0].color = XMFLOAT3{ 1, 1, 1 };
	instances[1].color = XMFLOAT3{ 1, 0, 0 };
	instances[2].color = XMFLOAT3{ 0, 1, 0 };
	instances[3].color = XMFLOAT3{ 0, 0, 1 };

	instances[0].position = XMFLOAT3{ 0, 0, 0 };
	instances[1].position = XMFLOAT3{ 30, 0, 0 };
	instances[2].position = XMFLOAT3{ 60, 0, 0 };
	instances[3].position = XMFLOAT3{ 90, 0, 0 };

	instanceBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	instanceBufferDesc.ByteWidth = sizeof(InstanceType) * m_instanceCount;
	instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	instanceBufferDesc.CPUAccessFlags = 0;
	instanceBufferDesc.MiscFlags = 0;
	instanceBufferDesc.StructureByteStride = 0;

	instanceData.pSysMem = instances;
	instanceData.SysMemPitch = 0;
	instanceData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&instanceBufferDesc, &instanceData, &m_instanceBuffer);
	if (FAILED(result))
	{
		return false;
	}

	delete instances;
	instances = 0;

	return true;
}
