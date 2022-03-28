
//
// Model.h
//

// Version 1.  Encapsulate the mesh contents of a CGModel imported via CGImport3.  Currently supports obj, 3ds or gsf files.  md2, md3 and md5 (CGImport4) untested.  For version 1 a single texture and sampler interface are associated with the Model.


#pragma once
#include <d3d11_2.h>
#include <DirectXMath.h>
#include <BaseModel.h>
#include <Animation.h>
#include <string>
#include <vector>
#include <cstdint>
#include <CBufferStructures.h>
#include <Utils.h>
#include <Camera.h>
#include <VertexStructures.h>

#include <Assimp\include\assimp\Importer.hpp>      // C++ importer interface
#include <Assimp\include\assimp\scene.h>           // Output data structure
#include <Assimp\include\assimp\postprocess.h>     // Post processing flags

class Texture;
class Material;
class Effect;
#define MAX_TEXTURES 8

class Model : public BaseModel {
	uint32_t							numMeshes = 0;
	std::vector<uint32_t>				indexCount;
	std::vector<uint32_t>				baseVertexOffset;

	HRESULT init(ID3D11Device *device) { return S_OK; };
	HRESULT loadModelAssimp(ID3D11Device *device, const std::wstring& filename);
	void load(ID3D11Device *device,  const std::wstring& filename);


public:

	Model(ID3D11Device *device, const std::wstring& filename, Effect *_effect, Material *_materials[] = nullptr, int _numMaterials = 0, ID3D11ShaderResourceView **textures = nullptr, int numTextures = 0) : BaseModel(device, _effect, _materials, _numMaterials, textures, numTextures){ load(device,  filename); }
	~Model();
	
	void render(ID3D11DeviceContext *context);
};
