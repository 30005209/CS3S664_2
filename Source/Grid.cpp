
#include <stdafx.h>
#include <Grid.h>
#include <Material.h>
using namespace std;
using namespace DirectX;
//using namespace DirectX::PackedVector;



HRESULT  Grid::init(ID3D11Device *device, UINT widthl, UINT heightl)//, Effect *_effect, ID3D11ShaderResourceView *tex_view, Material*_material) :Mesh(device, _effect, tex_view, _material){// XMCOLOR in_Diffuse, XMCOLOR in_Specular) {
{	
	Material material;
	if (numMaterials >= 1)
		material =*materials[0];

	width = widthl;
	height = heightl;
	numInd = ((width - 1) * 2 * 3)*(height - 1);
	ExtendedVertexStruct*vertices = (ExtendedVertexStruct*)malloc(sizeof(ExtendedVertexStruct)*width*height);
	UINT*indices = (UINT*)malloc(sizeof(UINT)*numInd);
	
	try
	{

		// Setup grid vertex buffer

		//INITIALISE Verticies
		for (int i = 0; i<height; i++)
		{
			for (int j = 0; j<width; j++)
			{

				vertices[(i*width) + j].pos.x = j;
				vertices[(i*width) + j].pos.z = i ;
				vertices[(i*width) + j].pos.y = 0;
				vertices[(i*width) + j].normal=XMFLOAT3(0, 1, 0);
				vertices[(i*width) + j].matDiffuse = material.getColour()->diffuse;
				vertices[(i*width) + j].matSpecular = material.getColour()->specular;
				vertices[(i*width) + j].texCoord.x = (float)j / width;
				vertices[(i*width) + j].texCoord.y = (float)i / height;
			}
		}



		// Setup vertex buffer
		D3D11_BUFFER_DESC vertexDesc;
		D3D11_SUBRESOURCE_DATA vertexData;

		ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

		vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexDesc.ByteWidth = sizeof(ExtendedVertexStruct) * width*height;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexData.pSysMem = vertices;

		HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);

		if (!SUCCEEDED(hr))
			throw exception("Vertex buffer cannot be created");


		// Also creates sampler - from baseModel
		D3D11_SAMPLER_DESC linearDesc;

		ZeroMemory(&linearDesc, sizeof(D3D11_SAMPLER_DESC));
		linearDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
		linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
		linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
		linearDesc.MinLOD = 0.0f;
		linearDesc.MaxLOD = 0.0f;
		linearDesc.MipLODBias = 0.0f;
		linearDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

		device->CreateSamplerState(&linearDesc, &cubeSampler);



		for (int i = 0; i<height - 1; i++)
		{
			for (int j = 0; j<width - 1; j++)
			{
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 0] = (i*width) + j;
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 2] = (i*width) + j + 1;
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 1] = ((i + 1)*width) + j;

				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 3] = (i*width) + j + 1;
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 5] = ((i + 1)*width) + j + 1;;
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 4] = ((i + 1)*width) + j;
			}
		}


		D3D11_BUFFER_DESC indexDesc;
		indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexDesc.ByteWidth = sizeof(UINT)* numInd;
		indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexDesc.CPUAccessFlags = 0;
		indexDesc.MiscFlags = 0;
		indexDesc.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA indexData;
		indexData.pSysMem = indices;
		
		hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);
		
		if (!SUCCEEDED(hr))
			throw exception("Vertex buffer cannot be created");

		// Dispose of local resources
		if (vertices)
			free(vertices);
		if (indices)
			free(indices);

	}
	catch (exception& e)
	{
		cout << "Grid object could not be instantiated due to:\n";
		cout << e.what() << endl;

		if (vertexBuffer)
			vertexBuffer->Release();
		if (indexBuffer)
			indexBuffer->Release();
		vertexBuffer = nullptr;
		indexBuffer = nullptr;
		return E_FAIL;
	}
	return S_OK;
}



Grid::~Grid() {

	if (vertexBuffer)
		vertexBuffer->Release();
	if (indexBuffer)
		indexBuffer->Release();
}


void Grid::render(ID3D11DeviceContext *context) {

	context->PSSetConstantBuffers(0, 1, &cBufferModelGPU);
	context->VSSetConstantBuffers(0, 1, &cBufferModelGPU);

	// Validate object before rendering 
	if (!context || !vertexBuffer )
		return;

	if (effect)
		// Sets shaders, states
		effect->bindPipeline(context);

	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if (numTextures > 0 && sampler) {

		//Standard sampler in slot 0
		context->PSSetShaderResources(0, numTextures, textures);
		context->PSSetSamplers(0, 1, &sampler);

		//Should cubeSampler exist - set it to slot 1
		if (cubeSampler)
			context->PSSetSamplers(1, 1, &cubeSampler);
	}

	// Set vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer };
	UINT vertexStrides[] = { sizeof(ExtendedVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);

	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw box object using index buffer
	// 36 indices for the box.
	context->DrawIndexed(numInd, 0, 0);

}