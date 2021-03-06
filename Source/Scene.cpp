

//
// Scene.cpp
//

#include <stdafx.h>
#include <string.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <System.h>
#include <DirectXTK\DDSTextureLoader.h>
#include <DirectXTK\WICTextureLoader.h>
#include <CGDClock.h>
#include <Scene.h>

#include <Effect.h>
#include <VertexStructures.h>
#include <Texture.h>
#include <BlurUtility.h>

//using namespace std;
//using namespace DirectX;
//using namespace DirectX::PackedVector;

//
// Methods to handle initialisation, update and rendering of the scene
HRESULT Scene::rebuildViewport(){
	// Binds the render target view and depth/stencil view to the pipeline.
	// Sets up viewport for the main window (wndHandle) 
	// Called at initialisation or in response to window resize
	ID3D11DeviceContext *context = system->getDeviceContext();
	if (!context)
		return E_FAIL;
	// Bind the render target view and depth/stencil view to the pipeline.
	ID3D11RenderTargetView* renderTargetView = system->getBackBufferRTV();
	context->OMSetRenderTargets(1, &renderTargetView, system->getDepthStencil());
	// Setup viewport for the main window (wndHandle)
	RECT clientRect;
	GetClientRect(wndHandle, &clientRect);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(clientRect.right - clientRect.left);
	viewport.Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	//Set Viewport
	context->RSSetViewports(1, &viewport);
	return S_OK;
}

// Main resource setup for the application.  These are setup around a given Direct3D device.
HRESULT Scene::initialiseSceneResources() {
	ID3D11DeviceContext *context = system->getDeviceContext();
	ID3D11Device *device = system->getDevice();
	if (!device)
		return E_FAIL;
	// Set up viewport for the main window (wndHandle) 
	rebuildViewport();

	// Setup main effects (pipeline shaders, states etc)
	// The Effect class is a helper class similar to the depricated DX9 Effect. It stores pipeline shaders, pipeline states  etc and binds them to setup the pipeline to render with a particular Effect. The constructor requires that at least shaders are provided along a description of the vertex structure.
	basicColourEffect = new Effect(device, "Shaders\\cso\\basic_colour_vs.cso", "Shaders\\cso\\basic_colour_ps.cso", basicVertexDesc, ARRAYSIZE(basicVertexDesc));
	basicLightingEffect = new Effect(device, "Shaders\\cso\\basic_lighting_vs.cso", "Shaders\\cso\\basic_colour_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	perPixelLightingEffect = new Effect(device, "Shaders\\cso\\per_pixel_lighting_vs.cso", "Shaders\\cso\\per_pixel_lighting_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	skyBoxEffect = new Effect(device, "Shaders\\cso\\sky_box_vs.cso", "Shaders\\cso\\sky_box_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	
	// Add Code Here ( Load reflection_map_vs.cso and reflection_map_ps.cso  )
	reflectionMappingEffect = new Effect(device, "Shaders\\cso\\reflection_map_vs.cso", "Shaders\\cso\\reflection_map_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	waterEffect = new Effect(device, "Shaders\\cso\\ocean_vs.cso", "Shaders\\cso\\ocean_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	
	grassEffect = new Effect(device, "Shaders\\cso\\grass_vs.cso", "Shaders\\cso\\grass_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	treeEffect = new Effect(device, "Shaders\\cso\\tree_vs.cso", "Shaders\\cso\\tree_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	fireEffect = new Effect(device, "Shaders\\cso\\fire_vs.cso", "Shaders\\cso\\fire_ps.cso", particleVertexDesc, ARRAYSIZE(particleVertexDesc));
	smokeEffect = new Effect(device, "Shaders\\cso\\fire_vs.cso", "Shaders\\cso\\fire_ps.cso", particleVertexDesc, ARRAYSIZE(particleVertexDesc));
	flareEffect = new Effect(device, "Shaders\\cso\\flare_vs.cso", "Shaders\\cso\\flare_ps.cso", flareVertexDesc, ARRAYSIZE(flareVertexDesc));

	//Blend States
	// FOILAGE
	ID3D11BlendState* foilageBSState = grassEffect->getBlendState();
	D3D11_BLEND_DESC foilageBSDesc;
	foilageBSState->GetDesc(&foilageBSDesc);

	foilageBSDesc.RenderTarget[0].BlendEnable = FALSE;
	foilageBSDesc.AlphaToCoverageEnable = TRUE;
	grassEffect->getBlendState()->Release();
	device->CreateBlendState(&foilageBSDesc, &foilageBSState);
	grassEffect->setBlendState(foilageBSState);
	treeEffect->setBlendState(foilageBSState);

	// FIRE
	ID3D11BlendState* fireBSState = fireEffect->getBlendState();
	D3D11_BLEND_DESC fireBSDesc;
	fireBSState->GetDesc(&fireBSDesc);
	fireBSDesc.RenderTarget[0].BlendEnable = TRUE;
	fireBSDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	fireBSDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	fireEffect->getBlendState()->Release();
	device->CreateBlendState(&fireBSDesc, &fireBSState);
	fireEffect->setBlendState(fireBSState);

	ID3D11DepthStencilState* fireDSState = fireEffect->getDepthStencilState();
	D3D11_DEPTH_STENCIL_DESC fireDSDesc;
	fireDSState->GetDesc(&fireDSDesc);
	fireDSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	fireEffect->getDepthStencilState()->Release();
	device->CreateDepthStencilState(&fireDSDesc, &fireDSState);
	fireEffect->setDepthStencilState(fireDSState);

	//FLARE
	ID3D11BlendState* flareBlendState = flareEffect->getBlendState();
	D3D11_BLEND_DESC flareBSDesc;
	flareBlendState->GetDesc(&flareBSDesc);
	flareBSDesc.AlphaToCoverageEnable = FALSE;
	flareBSDesc.RenderTarget[0].BlendEnable = TRUE;
	flareBSDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	flareBSDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	// Create custom flare blend state object
	flareBlendState->Release(); 
	device->CreateBlendState(&flareBSDesc, &flareBlendState);
	flareEffect->setBlendState(flareBlendState);

	// The Effect class constructor sets default depth/stencil, rasteriser and blend states
	// The Effect binds these states to the pipeline whenever an object using the effect is rendered
	// We can customise states if required
	ID3D11DepthStencilState *skyBoxDSState = skyBoxEffect->getDepthStencilState();
	D3D11_DEPTH_STENCIL_DESC skyBoxDSDesc;
	skyBoxDSState->GetDesc(&skyBoxDSDesc);
	skyBoxDSDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	skyBoxDSState->Release(); 
	device->CreateDepthStencilState(&skyBoxDSDesc, &skyBoxDSState);
	skyBoxEffect->setDepthStencilState(skyBoxDSState);

	// Setup Textures
	// The Texture class is a helper class to load textures

	heightMap = new Texture(device, L"Resources\\Textures\\heightmap.bmp");
	normalMap = new Texture(device, L"Resources\\Textures\\normalmap.bmp");

	cubeDayTexture = new Texture(device, L"Resources\\Textures\\grassenvmap1024.dds");
	waterNormalTexture = new Texture(device, L"Resources\\Textures\\Waves.dds");
	sharkTexture = new Texture(device, L"Resources\\Textures\\greatwhiteshark.png");
	castleTexture = new Texture(device, L"Resources\\Textures\\castle.jpg");

	// Foiliage / grass
	grassAlphaTexture = new Texture(device, L"Resources\\Textures\\grassAlpha.tif");
	grassDiffTexture = new Texture(device, L"Resources\\Textures\\grass.png");
	treeTexture = new Texture(device, L"Resources\\Textures\\tree.tif");

	brickTexture = new Texture(device, L"Resources\\Textures\\Brick_DIFFUSE.jpg");
	knightTexture = new Texture(device, L"Resources\\Textures\\knight_orig.jpg");

	//Fire
	fireTexture = new Texture(device, L"Resources\\Textures\\Fire.tif");
	smokeTexture = new Texture(device, L"Resources\\Textures\\smoke.tif");

	//Flare
	flare1Texture = new Texture(device, L"Resources\\Textures\\flares\\divine.png");
	flare2Texture = new Texture(device, L"Resources\\Textures\\flares\\extendring.png");


	// The BaseModel class supports multitexturing and the constructor takes a pointer to an array of shader resource views of textures. 
	// Even if we only need 1 texture/shader resource view for an effect we still need to create an array.
	ID3D11ShaderResourceView *skyBoxTextureArray[] = { cubeDayTexture->getShaderResourceView()};
	ID3D11ShaderResourceView* waterTextureArray[] = { waterNormalTexture->getShaderResourceView(), cubeDayTexture->getShaderResourceView()};
	ID3D11ShaderResourceView *brickTextureArray[] = { brickTexture->getShaderResourceView() };
	ID3D11ShaderResourceView* knightTextureArray[] = { knightTexture->getShaderResourceView()};
	ID3D11ShaderResourceView* sharkTextureArray[] = { sharkTexture->getShaderResourceView() };
	ID3D11ShaderResourceView* grassTextureArray[] = { grassDiffTexture->getShaderResourceView(), grassAlphaTexture->getShaderResourceView() };
	ID3D11ShaderResourceView* treeTextureArray[] = { treeTexture->getShaderResourceView() };
	ID3D11ShaderResourceView* castleTextureArray[] = { castleTexture->getShaderResourceView() };
	ID3D11ShaderResourceView* fireTextureArray[] = { fireTexture->getShaderResourceView() };
	ID3D11ShaderResourceView* smokeTextureArray[] = { smokeTexture->getShaderResourceView() }; 
	ID3D11ShaderResourceView* flare1TextureArray[] = { flare1Texture->getShaderResourceView() };
	ID3D11ShaderResourceView* flare2TextureArray[] = { flare2Texture->getShaderResourceView() };



	// Setup Objects - the object below are derived from the Base model class
	// The constructors for all objects derived from BaseModel require at least a valid pointer to the main DirectX device
	// And a valid effect with a vertex shader input structure that matches the object vertex structure.
	// In addition to the Texture array pointer mentioned above an additional optional parameters of the BaseModel class are a pointer to an array of Materials along with an int that contains the number of Materials
	// The baseModel class now manages a CBuffer containing model/world matrix properties. It has methods to update the cbuffers if the model/world changes 
	// The render methods of the objects attatch the world/model Cbuffers to the pipeline at slot b0 for vertex and pixel shaders
	
	// Create a skybox
	// The box class is derived from the BaseModel class 
	box = new Box(device, skyBoxEffect, NULL, 0, skyBoxTextureArray,1);
	
	// Add code here scale the box x1000
	box->setWorldMatrix(XMMatrixScaling(1000,1000,1000));
	box->update(context);

	// Create an orb model 
	// The Model class is also derived from the BaseModel class 
	orb0 = new Model(device, wstring(L"Resources\\Models\\sphere.3ds"), reflectionMappingEffect, NULL, 0, skyBoxTextureArray, 1);
	// Add code here scale the orb
	orb0->setWorldMatrix(XMMatrixScaling(2.0, 2.0, 2.0) * XMMatrixTranslation(-8, 0, 0));
	orb0->update(context);
	

	Material glossRed(XMCOLOR(1.0f, 0.0f, 0.0f, 1.0f));
	Material*glossRedMaterialArray[]{ &glossRed };
	Material matWhite;
	matWhite.setSpecular(XMCOLOR(0.2f, 0.2f, 0.2f, 0.01f));
	Material*matWhiteArray[]{ &matWhite };
	
	orb1 = new Model(device, wstring(L"Resources\\Models\\sphere.3ds"), perPixelLightingEffect,matWhiteArray, 1, brickTextureArray, 1);
	orb1->setWorldMatrix(XMMatrixScaling(0.5, 0.5, 0.5)*XMMatrixTranslation(-8, 3, 0));
	orb1->update(context);
	
	knight = new Model(device, wstring(L"Resources\\Models\\knight.3ds"), perPixelLightingEffect, matWhiteArray, 1, knightTextureArray, 1);
	knight->setWorldMatrix(XMMatrixScaling(0.02, 0.02, 0.02)* XMMatrixTranslation(2, -0.75f, 0));
	knight->update(context);

	shark = new Model(device, wstring(L"Resources\\Models\\shark.obj"), treeEffect, matWhiteArray, 1, sharkTextureArray, 1);
	shark->setWorldMatrix(XMMatrixScaling(0.25, 0.25, 0.25) * XMMatrixTranslation(-5, -0.75f, 0));
	shark->update(context);

	castle = new Model(device, wstring(L"Resources\\Models\\castle.3DS"), perPixelLightingEffect, matWhiteArray, 1, castleTextureArray, 1);
	castle->setWorldMatrix(XMMatrixRotationY(90) * XMMatrixScaling(10.0f, 10.0f, 10.0f) * XMMatrixTranslation(-10, 0, 20));
	castle->update(context);
		
	//OLD Grid based Grass
	//grass = new Grid(20, 20, device, grassEffect, matWhiteArray, 1, grassTextureArray, 2);
	//grass->setWorldMatrix(XMMatrixScaling(5, 5, 5) * XMMatrixTranslation(-10, 0, 0));
	//grass->update(context);

	grass = new Terrain(device, context, 100, 100, heightMap->getTexture(), normalMap->getTexture(), grassEffect, matWhiteArray, 1, grassTextureArray, 2);
	grass->setWorldMatrix(XMMatrixScaling(1, 2, 1) *XMMatrixTranslation(-50.0f,0.0f,-50.0f));
	grass->update(context);

	// Water init - final int is number of textures
	water = new Grid(32, 30, device, waterEffect, matWhiteArray, 1, waterTextureArray, 2);
	water->setWorldMatrix(XMMatrixScaling(1, 1, 1)* XMMatrixTranslation(-25, grass->CalculateYValueWorld(5, 5)+.01f, -15));
	water->update(context);

	tree0 = new Model(device, wstring(L"Resources\\Models\\tree.3DS"), treeEffect, matWhiteArray, 1, treeTextureArray, 1);
	tree0->setWorldMatrix(XMMatrixTranslation(-30, grass->CalculateYValueWorld(-30, 10), 10));
	tree0->update(context); 

	tree1 = new Model(device, wstring(L"Resources\\Models\\tree.3DS"), treeEffect, matWhiteArray, 1, treeTextureArray, 1);
	tree1->setWorldMatrix(XMMatrixTranslation(-20, grass->CalculateYValueWorld(-20,10), 10));
	tree1->update(context);

	tree2 = new Model(device, wstring(L"Resources\\Models\\tree.3DS"), treeEffect, matWhiteArray, 1, treeTextureArray, 1);
	tree2->setWorldMatrix(XMMatrixTranslation(-30, grass->CalculateYValueWorld(-30, 20), 20));
	tree2->update(context);
	
	fire = new ParticleSystem(device, fireEffect, matWhiteArray, 1, fireTextureArray, 1);
	fire->setWorldMatrix(XMMatrixTranslation(10, 1.0f, 0));
	smoke = new ParticleSystem(device, fireEffect, matWhiteArray, 1, smokeTextureArray, 1);

	// Create Flares
	for (int i = 0; i < numFlares; i++)
	{
		if (randM1P1() > 0)
			flares[i] = new Flare(XMFLOAT3(-125.0f, 60.0f, 70.0f), XMCOLOR(randM1P1() * 0.5f + 0.5f, randM1P1() * 0.5f + 0.5f, randM1P1() * 0.5f + 0.5f, (float)i / numFlares), device, flareEffect, NULL, 0, flare1TextureArray, 1);
		else
			flares[i] = new Flare(XMFLOAT3(-125.0f, 60.0f, 70.0f), XMCOLOR(randM1P1() * 0.5f + 0.5f, randM1P1() * 0.5f + 0.5f, randM1P1() * 0.5f + 0.5f, (float)i / numFlares), device, flareEffect, NULL, 0, flare2TextureArray, 1);
	}

	glow = new BlurUtility(system->getDevice(), context, 256, 256);


	// Setup a camera
	// The LookAtCamera is derived from the base Camera class. The constructor for the Camera class requires a valid pointer to the main DirectX device
	// and and 3 vectors to define the initial position, up vector and target for the camera.
	// The camera class  manages a Cbuffer containing view/projection matrix properties. It has methods to update the cbuffers if the camera moves changes  
	// The camera constructor and update methods also attaches the camera CBuffer to the pipeline at slot b1 for vertex and pixel shaders
	mainCamera = new FirstPersonCamera(device, XMVectorSet(15, 2.0, -5.0, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), XMVectorSet(0.8f, 0.0f, -1.0f, 1.0f));
	mainCamera->turn(1.25f);
	mainCamera->setFlying(false);

	// Add a CBuffer to store light properties - you might consider creating a Light Class to manage this CBuffer
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferLight
	cBufferLightCPU = (CBufferLight *)_aligned_malloc(sizeof(CBufferLight), 16);

	// Fill out cBufferLightCPU
	cBufferLightCPU->lightVec = DirectX::XMFLOAT4(-5.0f, 2.0f, 5.0f, 1.0f);
	cBufferLightCPU->lightAmbient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	cBufferLightCPU->lightDiffuse = DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	cBufferLightCPU->lightSpecular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// Create GPU resource memory copy of cBufferLight
	// fill out description (Note if we want to update the CBuffer we need  D3D11_CPU_ACCESS_WRITE)
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	cbufferDesc.ByteWidth = sizeof(CBufferLight);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferLightCPU;// Initialise GPU CBuffer with data from CPU CBuffer

	HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData,
		&cBufferLightGPU);

	// We dont strictly need to call map here as the GPU CBuffer was initialised from the CPU CBuffer at creation.
	// However if changes are made to the CPU CBuffer during update the we need to copy the data to the GPU CBuffer 
	// using the mapCbuffer helper function provided the in Util.h   

	mapCbuffer(context, cBufferLightCPU, cBufferLightGPU, sizeof(CBufferLight));
	context->VSSetConstantBuffers(2, 1, &cBufferLightGPU);// Attach CBufferLightGPU to register b2 for the vertex shader. Not strictly needed as our vertex shader doesnt require access to this CBuffer
	context->PSSetConstantBuffers(2, 1, &cBufferLightGPU);// Attach CBufferLightGPU to register b2 for the Pixel shader.

	// Add a Cbuffer to stor world/scene properties
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferLight
	cBufferSceneCPU = (CBufferScene *)_aligned_malloc(sizeof(CBufferScene), 16);

	// Fill out cBufferSceneCPU
	cBufferSceneCPU->windDir = DirectX::XMFLOAT4(1, 0, 0, 1);
	cBufferSceneCPU->Time = 0.0;
	cBufferSceneCPU->grassHeight = 0.0;
	
	cbufferInitData.pSysMem = cBufferSceneCPU;// Initialise GPU CBuffer with data from CPU CBuffer
	cbufferDesc.ByteWidth = sizeof(CBufferScene);

	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferSceneGPU);

	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	context->VSSetConstantBuffers(3, 1, &cBufferSceneGPU);// Attach CBufferSceneGPU to register b3 for the vertex shader. Not strictly needed as our vertex shader doesnt require access to this CBuffer
	context->PSSetConstantBuffers(3, 1, &cBufferSceneGPU);// Attach CBufferSceneGPU to register b3 for the Pixel shader

	return S_OK;
}

// Update scene state (perform animations etc)
HRESULT Scene::updateScene(ID3D11DeviceContext *context,Camera *camera) {

	// mainClock is a helper class to manage game time data
	mainClock->tick();
	static float timer = 5.0f;
	timer -= mainClock->gameTimeDelta();
	if (timer < 0)
	{
		cout << "Average FPS: " << mainClock->averageFPS() << endl;
		cout << "Average SPF: " << mainClock->averageSPF() << endl;
		timer = 5.0f;
	}
	double dT = mainClock->gameTimeDelta();
	double gT = mainClock->gameTimeElapsed();
	//cout << "Game time Elapsed= " << gT << " seconds" << endl;

	// If the CPU CBuffer contents are changed then the changes need to be copied to GPU CBuffer with the mapCbuffer helper function
	mainCamera->update(context);

	orb1->setWorldMatrix(orb1->getWorldMatrix() * XMMatrixRotationZ((float)dT));
	orb1->update(context);

	knight->setWorldMatrix(knight->getWorldMatrix());
	knight->update(context);
	
	water->update(context);
	
	shark->setWorldMatrix(shark->getWorldMatrix() * XMMatrixRotationY(dT / 2.0f));
	shark->update(context);

	fire->update(context);

	tree0->setWorldMatrix(XMMatrixTranslation(10, grass->CalculateYValueWorld(10, 10), 10));
	tree0->setWorldMatrix(XMMatrixTranslation(50, grass->CalculateYValueWorld(10, 10), 10));
	tree0->setWorldMatrix(XMMatrixTranslation(20, grass->CalculateYValueWorld(10, 10), 50));

	//OBJ->setWorldMatrix(OBJ->getWorldMatrix() * XMMatrixTranslation(0, 0,0));

	//goat->setWorldMatrix(goat->getWorldMatrix() * XMMatrixTranslation(0.0f, (float)gT * 0.01f, 0.0f) * XMMatrixRotationY((float)-gT/ 2.0f));


	// Update the scene time as it is needed to animate the water
	cBufferSceneCPU->Time = (float)gT;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	
	return S_OK;
}

// Render scene
HRESULT Scene::renderScene() {

	ID3D11DeviceContext *context = system->getDeviceContext();
	
	// Validate window and D3D context
	if (isMinimised() || !context)
		return E_FAIL;
	
	// Clear the screen
	static const FLOAT clearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	context->ClearRenderTargetView(system->getBackBufferRTV(), clearColor);
	context->ClearDepthStencilView(system->getDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render Scene objects
	
	// Render SkyBox
	if (box)
		box->render(context);
	
	// Render orb
	if (orb0)
	{
		orb0->render(context);
	}
	
	if (orb1)
	{
		glow->blurModel(orb1, system->getDepthStencilSRV());
		orb1->render(context);
	}
	
	// Render goat
	if (knight)
	{
		knight->render(context);
	}
	
	if (shark)
		shark->render(context);

	if (water)
		water->render(context);
	
	if (castle)
		castle->render(context);
	
	if (grass)
	{
		for (int i = 0; i < numGrassPasses; i++)
		{
			cBufferSceneCPU->grassHeight = (grassLength / numGrassPasses) * i;
			mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
			grass->render(context);
		}
	}
	
	if (tree0)
		tree0->render(context);
	
	if (tree1)
		tree1->render(context);
	
	if (tree2)
		tree2->render(context);
	
	if (fire)
		fire->render(context);
	
	if (smoke)
		smoke->render(context);

	DrawFlare(context);

	// Present current frame to the screen
	HRESULT hr = system->presentBackBuffer();

	return S_OK;
}

//
// Event handling methods
//
// Process mouse move with the left button held down
void Scene::handleMouseLDrag(const POINT &disp) {
	//LookAtCamera

	//mainCamera->rotateElevation((float)-disp.y * 0.01f);
	//mainCamera->rotateOnYAxis((float)-disp.x * 0.01f);

	//FirstPersonCamera
	mainCamera->elevate((float)-disp.y * 0.01f);
	mainCamera->turn((float)-disp.x * 0.01f);
}

// Process mouse wheel movement
void Scene::handleMouseWheel(const short zDelta) {
	//LookAtCamera

	//if (zDelta<0)
	//	mainCamera->zoomCamera(1.2f);
	//else if (zDelta>0)
	//	mainCamera->zoomCamera(0.9f);
	//cout << "zoom" << endl;
	//FirstPersonCamera
	mainCamera->move(zDelta*0.01f);
}

// Process key down event.  keyCode indicates the key pressed while extKeyFlags indicates the extended key status at the time of the key down event (see http://msdn.microsoft.com/en-gb/library/windows/desktop/ms646280%28v=vs.85%29.aspx).
void Scene::handleKeyDown(const WPARAM keyCode, const LPARAM extKeyFlags) {
	// Add key down handler here...
	if (keyCode == VK_HOME)
		mainCamera->elevate(0.05f);

	if (keyCode == VK_END)
		mainCamera->elevate(-0.05f);

	if (keyCode == VK_LEFT)
		mainCamera->turn(-0.05f);

	if (keyCode == VK_RIGHT)
		mainCamera->turn(0.05f);

	if (keyCode == VK_SPACE)
	{
		bool isFlying = mainCamera->toggleFlying();
		if (isFlying)
			cout << "Flying mode is on" << endl;
		else
			cout << "Flying mode is off" << endl;
	}
	if (keyCode == VK_UP)
		mainCamera->move(0.5);

	if (keyCode == VK_DOWN)
		mainCamera->move(-0.5);

}

// Process key up event.  keyCode indicates the key released while extKeyFlags indicates the extended key status at the time of the key up event (see http://msdn.microsoft.com/en-us/library/windows/desktop/ms646281%28v=vs.85%29.aspx).
void Scene::handleKeyUp(const WPARAM keyCode, const LPARAM extKeyFlags) {
	// Add key up handler here...
}






// Clock handling methods
void Scene::startClock() {
	mainClock->start();
}

void Scene::stopClock() {
	mainClock->stop();
}

void Scene::reportTimingData() {

	cout << "Actual time elapsed = " << mainClock->actualTimeElapsed() << endl;
	cout << "Game time elapsed = " << mainClock->gameTimeElapsed() << endl << endl;
	mainClock->reportTimingData();
}

// Private constructor
Scene::Scene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc) {
	try
	{
		// 1. Register window class for main DirectX window
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
		wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = wndClassName;
		wcex.hIconSm = NULL;
		if (!RegisterClassEx(&wcex))
			throw exception("Cannot register window class for Scene HWND");
		// 2. Store instance handle in our global variable
		hInst = hInstance;
		// 3. Setup window rect and resize according to set styles
		RECT		windowRect;
		windowRect.left = 0;
		windowRect.right = _width;
		windowRect.top = 0;
		windowRect.bottom = _height;
		DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;
		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);
		// 4. Create and validate the main window handle
		wndHandle = CreateWindowEx(dwExStyle, wndClassName, wndTitle, dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 500, 500, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL, NULL, hInst, this);
		if (!wndHandle)
			throw exception("Cannot create main window handle");
		ShowWindow(wndHandle, nCmdShow);
		UpdateWindow(wndHandle);
		SetFocus(wndHandle);
		// 5. Create DirectX host environment (associated with main application wnd)
		system = System::CreateDirectXSystem(wndHandle);
		if (!system)
			throw exception("Cannot create Direct3D device and context model");
		// 6. Setup application-specific objects
		HRESULT hr = initialiseSceneResources();
		if (!SUCCEEDED(hr))
			throw exception("Cannot initalise scene resources");
		// 7. Create main clock / FPS timer (do this last with deferred start of 3 seconds so min FPS / SPF are not skewed by start-up events firing and taking CPU cycles).
		mainClock = CGDClock::CreateClock(string("mainClock"), 3.0f);
		if (!mainClock)
			throw exception("Cannot create main clock / timer");
	}
	catch (exception &e)
	{
		cout << e.what() << endl;
		// Re-throw exception
		throw;
	}
}

// Helper function to call updateScene followed by renderScene
HRESULT Scene::updateAndRenderScene() {
	ID3D11DeviceContext *context = system->getDeviceContext();
	HRESULT hr = updateScene(context, (Camera*)mainCamera);
	if (SUCCEEDED(hr))
		hr = renderScene();

	if (!mainCamera->getFlying())//if not flying stick to ground
	{
		XMVECTOR camPos = mainCamera->getPos();
		float camHeight = 2;//grass->CalculateYValueWorld(camPos.vector4_f32[0],
			//camPos.vector4_f32[2]);
		mainCamera->setHeight(camHeight);
	}

	return hr;
}

// Return TRUE if the window is in a minimised state, FALSE otherwise
BOOL Scene::isMinimised() {

	WINDOWPLACEMENT				wp;

	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);

	return (GetWindowPlacement(wndHandle, &wp) != 0 && wp.showCmd == SW_SHOWMINIMIZED);
}

//
// Public interface implementation
//
// Method to create the main Scene instance
Scene* Scene::CreateScene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc) {
	static bool _scene_created = false;
	Scene *scene = nullptr;
	if (!_scene_created) {
		scene = new Scene(_width, _height, wndClassName, wndTitle, nCmdShow, hInstance, WndProc);
		if (scene)
			_scene_created = true;
	}
	return scene;
}

// Destructor
Scene::~Scene() {
	
	//Clean Up

	if (cBufferSceneCPU)
		_aligned_free(cBufferSceneCPU);
	if (cBufferLightCPU)
		_aligned_free(cBufferLightCPU);
	if (cBufferSceneGPU)
		cBufferSceneGPU->Release();
	if (cBufferLightGPU)
		cBufferLightGPU->Release();
	
	// Delete Scene Effects
	if (basicColourEffect)
		delete(basicColourEffect);
	if (basicLightingEffect)
		delete(basicLightingEffect);
	if (perPixelLightingEffect)
		delete(perPixelLightingEffect);
	if (skyBoxEffect)
		delete(skyBoxEffect);
	if (reflectionMappingEffect)
		delete(reflectionMappingEffect);
	if (waterEffect)
		delete waterEffect;
	if (grassEffect)
		delete grassEffect;
	if (treeEffect)
		delete treeEffect;
	if (fireEffect)
		delete fireEffect;
	if (smokeEffect)
		delete smokeEffect;
	if (flareEffect)
		delete flareEffect;

	// Delete Textures
	if (heightMap)
		delete heightMap;
	if (normalMap)
		delete normalMap;
	if (cubeDayTexture)
		delete cubeDayTexture;
	if (waterNormalTexture)
		delete waterNormalTexture;
	if (sharkTexture)
		delete sharkTexture;
	if (castleTexture)
		delete castleTexture;
	if (grassAlphaTexture)
		delete grassAlphaTexture;
	if (grassDiffTexture)
		delete grassDiffTexture;
	if (treeTexture)
		delete treeTexture;
	if (brickTexture)
		delete brickTexture;
	if (knightTexture)
		delete knightTexture;
	if (fireTexture)
		delete fireTexture;
	if (smokeTexture)
		delete smokeTexture;
	if (flare1Texture)
		delete flare1Texture;
	if (flare2Texture)
		delete flare2Texture;


	// Delete Scene objects

	if (box)
		delete(box);
	if (orb0)
		delete(orb0);
	if (orb1)
		delete(orb1);
	if (knight)
		delete(knight);
	if (shark)
		delete(shark);
	if (castle)
		delete(castle);
	if (water)
		delete(water);
	if (grass)
		delete(grass);
	if(tree0)
		delete(tree0);
	if(tree1)
		delete(tree1);
	if (tree2)
		delete(tree2);
	if (fire)
		delete(fire);
	if (smoke)
		delete(smoke);
	if (flares)
	{
		for (int i = 0; i < numFlares; i++)
		{
			delete flares[i];
		}
	}
	if (glow)
		delete glow;
	if (mainClock)
		delete(mainClock);
	if (mainCamera)
		delete(mainCamera);	
	if (system)
		delete(system);

	if (wndHandle)
		DestroyWindow(wndHandle);
	//done
}

// Call DestoryWindow on the HWND
void Scene::destoryWindow() {
	if (wndHandle != NULL) {
		HWND hWnd = wndHandle;
		wndHandle = NULL;
		DestroyWindow(hWnd);
	}
}

//
// Private interface implementation
//
// Resize swap chain buffers and update pipeline viewport configurations in response to a window resize event
HRESULT Scene::resizeResources() {
	if (system) {
		// Only process resize if the System *system exists (on initial resize window creation this will not be the case so this branch is ignored)
		HRESULT hr = system->resizeSwapChainBuffers(wndHandle);
		rebuildViewport();
		RECT clientRect;
		GetClientRect(wndHandle, &clientRect);
		if (!isMinimised())
			renderScene();
	}
	return S_OK;
}

void Scene::DrawFlare(ID3D11DeviceContext* context)
{
	// Draw the Fire (Draw all transparent objects last)
	if (flares) {

		ID3D11RenderTargetView* tempRT[1] = { 0 };
		ID3D11DepthStencilView* tempDS = nullptr;

		// Set NULL depth buffer so we can also use the Depth Buffer as a shader
		// resource this is OK as we don?t need depth testing for the flares

		ID3D11DepthStencilView* nullDSV[1]; nullDSV[0] = NULL;
		context->OMGetRenderTargets(1, tempRT, &tempDS);
		context->OMSetRenderTargets(1, tempRT, NULL);
		ID3D11ShaderResourceView* depthSRV = system->getDepthStencilSRV();
		context->VSSetShaderResources(1, 1, &depthSRV);

		for (int i = 0; i < numFlares; i++)
			flares[i]->render(context);

		// Use Null SRV to release depth shader resource so it is available for writing
		ID3D11ShaderResourceView* nullSRV[1]; nullSRV[0] = NULL;
		context->VSSetShaderResources(1, 1, nullSRV);
		// Return default (read and write) depth buffer view.
		context->OMSetRenderTargets(1, tempRT, tempDS);

	}
}
