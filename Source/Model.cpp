
#include "stdafx.h"
#include "Model.h"
#include <Material.h>
#include <Effect.h>
#include <iostream>
#include <exception>

#include <CoreStructures\CoreStructures.h>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace CoreStructures;


void Model::load(ID3D11Device *device,  const std::wstring& filename) {

	try
	{
		if (!device )
			throw exception("Invalid parameters for Model instantiation");

		HRESULT hr=loadModelAssimp(device, filename);

		if (!SUCCEEDED(hr))
			throw exception("Cannot create input layout interface");
	
	}
	catch (exception& e)
	{
		cout << "Model could not be instantiated due to:\n";
		cout << e.what() << endl;

		if (vertexBuffer)
			vertexBuffer->Release();

		if (indexBuffer)
			indexBuffer->Release();

		vertexBuffer = nullptr;
		indexBuffer = nullptr;
		numMeshes = 0;
	}
}

Model::~Model() {

	if (vertexBuffer)
		vertexBuffer->Release();

	if (indexBuffer)
		indexBuffer->Release();

}


void Model::render(ID3D11DeviceContext *context) {//, int mode

	// Validate Model before rendering (see notes in constructor)
	if (!context || !vertexBuffer || !indexBuffer || !effect)
		return;

	effect->bindPipeline(context);

	// Set Model vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer };
	UINT vertexStrides[] = { sizeof(ExtendedVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if (numTextures>0 && sampler) {

		context->PSSetShaderResources(0, numTextures, textures);
		context->PSSetSamplers(0, 1, &sampler);
	}
	context->VSSetConstantBuffers(0, 1, &cBufferModelGPU);
	context->PSSetConstantBuffers(0, 1, &cBufferModelGPU);

	// Draw Model
	for (uint32_t indexOffset = 0, i = 0; i < numMeshes; indexOffset += indexCount[i], ++i)
		context->DrawIndexed(indexCount[i], indexOffset, baseVertexOffset[i]);	
	
}



HRESULT Model::loadModelAssimp(ID3D11Device *device, const std::wstring& filename)
{
	ExtendedVertexStruct *_vertexBuffer = nullptr;
	uint32_t *_indexBuffer = nullptr;

	Assimp::Importer importer;
	std::wstring w(filename); 
	std::string filename_string(w.begin(), w.end());
	// Get filename extension
	wstring ext = filename.substr(filename.length() - 4);
	
	Material material;

	if (numMaterials != 0)
		material = *materials[0];
	

	try
	{
		const aiScene* scene = importer.ReadFile(filename_string, aiProcess_PreTransformVertices| aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType);


		if (!scene)
		{
			printf("Couldn't load model - Error Importing Asset");
			return false;
		}

		numMeshes = scene->mNumMeshes;

		if (numMeshes == 0)
			throw exception("Empty model loaded");

		uint32_t numVertices = 0;
		uint32_t numIndices = 0;
		//printf("Num Meshes %d\n", scene->mNumMeshes);
		for (uint32_t k = 0; k < numMeshes; ++k)
		{
			aiMesh* mesh = scene->mMeshes[k];
			//Store base vertex index;
			baseVertexOffset.push_back(numVertices);
			// Increment vertex count
			numVertices += mesh->mNumVertices;
			// Store num indices for current mesh
			indexCount.push_back(mesh->mNumFaces * 3);
			numIndices += mesh->mNumFaces * 3;

		}

		// Create vertex buffer
		_vertexBuffer = (ExtendedVertexStruct*)malloc(numVertices * sizeof(ExtendedVertexStruct));

			if (!_vertexBuffer)
				throw exception("Cannot create vertex buffer");

			// Create index buffer
			_indexBuffer = (uint32_t*)malloc(numIndices * sizeof(uint32_t));

			if (!_indexBuffer)
				throw exception("Cannot create index buffer");

			// Copy vertex data into single buffer
			ExtendedVertexStruct *vptr = _vertexBuffer;
			uint32_t *indexPtr = _indexBuffer;

			for (uint32_t i = 0; i < numMeshes; ++i) 
			{

				aiMesh* mesh = scene->mMeshes[i];

				uint32_t j = 0;
				for ( j = 0; j < mesh->mNumFaces; ++j)		
				{
					const aiFace& face = mesh->mFaces[j];
					for (int k = 0; k < 3; ++k)
					{
						int VIndex = baseVertexOffset[i] + face.mIndices[k];
						aiVector3D pos = mesh->mVertices[face.mIndices[k]];
						aiVector3D uv = mesh->mTextureCoords[0][face.mIndices[k]];
						aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[face.mIndices[k]] : aiVector3D(1.0f, 1.0f, 1.0f);
						//Flip normal.x for OBJ & GSF (might be required for other files too?)
						if (0 == ext.compare(L".obj") || 0 == ext.compare(L".gsf"))
						{
							normal.x = -normal.x;
							pos.x = -pos.x;
						}
						vptr[VIndex].pos = XMFLOAT3(pos.x, pos.y, pos.z);
						vptr[VIndex].normal = XMFLOAT3(normal.x, normal.y, normal.z);
						vptr[VIndex].texCoord = XMFLOAT2(uv.x, 1-uv.y);
						vptr[VIndex].matDiffuse = material.getColour()->diffuse;//XMCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
						vptr[VIndex].matSpecular = material.getColour()->specular;// XMCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
						indexPtr[0] = face.mIndices[k];
						indexPtr++;

					}
				}//for each face

			}//for each mesh

	
			// Setup DX vertex buffer interfaces
			D3D11_BUFFER_DESC vertexDesc;
			D3D11_SUBRESOURCE_DATA vertexData;

			ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
			ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

			vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
			vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexDesc.ByteWidth = numVertices * sizeof(ExtendedVertexStruct);
			vertexData.pSysMem = _vertexBuffer;

			HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);

			if (!SUCCEEDED(hr))
				throw exception("Vertex buffer cannot be created");

			// Setup index buffer
			D3D11_BUFFER_DESC indexDesc;
			D3D11_SUBRESOURCE_DATA indexData;

			ZeroMemory(&indexDesc, sizeof(D3D11_BUFFER_DESC));
			ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));

			indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
			indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			indexDesc.ByteWidth = numIndices * sizeof(uint32_t);
			indexData.pSysMem = _indexBuffer;

			hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

			if (!SUCCEEDED(hr))
				throw exception("Index buffer cannot be created");

			// Dispose of local resources
			if (_vertexBuffer)
			free(_vertexBuffer);
			if (_vertexBuffer)
			free(_indexBuffer);

			//printf("done\n");

		}
		catch (exception& e)
		{
			cout << "Model could not be instantiated due to:\n";
			cout << e.what() << endl;

			if (_vertexBuffer)
				free(_vertexBuffer);

			if (_indexBuffer)
				free(_indexBuffer);

			return-1;
		}

	return 0;
}

