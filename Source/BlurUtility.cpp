#include "stdafx.h"
#include "BlurUtility.h"
#include <Model.h>
#include <Quad.h>
#include <Effect.h>
#include <VertexStructures.h>

BlurUtility::BlurUtility(ID3D11Device *deviceIn, ID3D11DeviceContext *contextIn, int _blurWidth, int _blurHeight)
{
	device = deviceIn;
	context = contextIn;
	
	initCBuffer(device, _blurWidth, _blurHeight);
	setupBlurRenderTargets(deviceIn, _blurWidth,  _blurHeight);
	char *tmpShaderBytecode = nullptr;

	ID3D11InputLayout	*screenQuadVSInputLayout = nullptr;
	SIZE_T shaderBytes = LoadShader("Shaders\\cso\\screen_quad_vs.cso", &tmpShaderBytecode);
	device->CreateVertexShader(tmpShaderBytecode, shaderBytes, NULL, &screenQuadVS);
	device->CreateInputLayout(basicVertexDesc, ARRAYSIZE(basicVertexDesc), tmpShaderBytecode, shaderBytes, &screenQuadVSInputLayout);
	free(tmpShaderBytecode);
	screenQuad = new Quad(device, screenQuadVSInputLayout);
	shaderBytes=LoadShader("Shaders\\cso\\per_pixel_lighting_vs.cso", &tmpShaderBytecode);
	device->CreateVertexShader(tmpShaderBytecode, shaderBytes, NULL, &perPixelLightingVS);
	free(tmpShaderBytecode);
	shaderBytes = LoadShader("Shaders\\cso\\convolve_u_ps.cso", &tmpShaderBytecode);
	device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &horizontalBlurPS);
	free(tmpShaderBytecode);
	shaderBytes = LoadShader("Shaders\\cso\\convolve_v_ps.cso", &tmpShaderBytecode);
	device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &verticalBlurPS);
	free(tmpShaderBytecode);
	shaderBytes = LoadShader("Shaders\\cso\\emissive_ps.cso", &tmpShaderBytecode);
	device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &emissivePS);
	free(tmpShaderBytecode);
	shaderBytes = LoadShader("Shaders\\cso\\copy_ps.cso", &tmpShaderBytecode);
	device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &textureCopyPS);
	free(tmpShaderBytecode);
	shaderBytes = LoadShader("Shaders\\cso\\copy_depth_ps.cso", &tmpShaderBytecode);
	device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &depthCopyPS);
	free(tmpShaderBytecode);
	
	defaultEffect = new Effect(device, "Shaders\\cso\\basic_colour_vs.cso", "Shaders\\cso\\basic_colour_ps.cso", basicVertexDesc, ARRAYSIZE(basicVertexDesc));
	
	D3D11_BLEND_DESC blendDesc;
	defaultEffect->getBlendState()->GetDesc(&blendDesc);//the effect is initialised with the default blend state
	blendDesc.AlphaToCoverageEnable = FALSE; // Use pixel coverage info from rasteriser (default)
	blendDesc.RenderTarget[0].BlendEnable =TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	device->CreateBlendState(&blendDesc, &alphaOnBlendState);


	// If textures are used a sampler is required for the pixel shader to sample the texture
	D3D11_SAMPLER_DESC linearDesc;

	ZeroMemory(&linearDesc, sizeof(D3D11_SAMPLER_DESC));
	linearDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	//linearDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.MinLOD = 0.0f;
	linearDesc.MaxLOD = 0.0f;
	linearDesc.MipLODBias = 0.0f;
	linearDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	device->CreateSamplerState(&linearDesc, &sampler);

}
void BlurUtility::initCBuffer(ID3D11Device *device, int _blurWidth, int _blurHeight) {

	// Allocate 16 byte aligned block of memory for "main memory" copy of CBufferTextSize
	cBufferTextSizeCPU = (CBufferTextSize*)_aligned_malloc(sizeof(CBufferTextSize), 16);

	// Fill out cBufferTextSizeCPU
	cBufferTextSizeCPU->Width = _blurWidth;
	cBufferTextSizeCPU->Height = _blurHeight;

	// Create GPU resource memory copy of CBufferTextSize
	// fill out description (Note if we want to update the CBuffer we need  D3D11_CPU_ACCESS_WRITE)
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	cbufferDesc.ByteWidth = sizeof(CBufferTextSize);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferTextSizeCPU;// Initialise GPU CBuffer with data from CPU CBuffer

	HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData,
		&cBufferTextSizeGPU);
}

void BlurUtility::updateTextSize(ID3D11DeviceContext *context) {
	mapCbuffer(context, cBufferTextSizeCPU, cBufferTextSizeGPU, sizeof(CBufferTextSize));
	context->PSSetConstantBuffers(0, 1, &cBufferTextSizeGPU);
	context->VSSetConstantBuffers(0, 1, &cBufferTextSizeGPU);
}
void BlurUtility::blurModel(Model*obj, ID3D11ShaderResourceView	*depthSRV)
{
	// Blur the object 
	if (obj) {

		// Ensure default states
		FLOAT			blendFactor[] = { 1, 1, 1, 1 };
		context->OMSetDepthStencilState(defaultEffect->getDepthStencilState(), 0);
		context->OMSetBlendState(defaultEffect->getBlendState(), blendFactor, 0xFFFFFFFF);

		// Store Scene render target and depth buffer to restore when finished
		ID3D11RenderTargetView		* tempRT[1] = { 0 };
		ID3D11DepthStencilView		*tempDS = nullptr;
		context->OMGetRenderTargets(1, tempRT, &tempDS);

		// Clear off screen render targets
		FLOAT clearColorBlur[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		context->ClearRenderTargetView(intermedRTV, clearColorBlur);
		context->ClearRenderTargetView(intermedHRTV, clearColorBlur);
		context->ClearRenderTargetView(intermedVRTV, clearColorBlur);
		context->ClearDepthStencilView(depthStencilViewBlur, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// Store Scene viewport to put them back when finished
		D3D11_VIEWPORT 		currentVP;
		UINT nCurrentVP = 1;
		context->RSGetViewports(&nCurrentVP, &currentVP);
		// Set Viewport Offscreen 
		context->RSSetViewports(1, &offScreenViewport);
		
		//The next step is required because the application depth buffer is multi sampled
		//copy multisample depth stencil (depthSRV) to non-ms depth stencil (depthStencilViewBlur)
		cBufferTextSizeCPU->Width = currentVP.Width;
		cBufferTextSizeCPU->Height = currentVP.Height;
		updateTextSize(context);
		context->PSSetConstantBuffers(0, 1, &cBufferTextSizeGPU);
		//Create null Render Target Array
		ID3D11RenderTargetView		*nullRT[1] = { 0 };
		context->OMSetRenderTargets(1, nullRT, depthStencilViewBlur);
		context->PSSetShaderResources(0, 1, &depthSRV);
		// Set depth copy vertex and pixel shaders
		context->VSSetShader(screenQuadVS, 0, 0);
		context->PSSetShader(depthCopyPS, 0, 0);
		screenQuad->render(context);

		//Render object to be blured offscreen
		context->OMSetRenderTargets(1, &intermedRTV, depthStencilViewBlur);
		XMMATRIX objMat = obj->getWorldMatrix();
		obj->setWorldMatrix(XMMatrixScaling(1.1, 1.1,1.1)*objMat);
		obj->update(context);
		obj->render(context);// Render using objects Effect
		obj->setWorldMatrix(objMat);
		obj->update(context);

		//Blur of screen render target in x2 passes
		context->PSSetSamplers(0, 1, &sampler);
		cBufferTextSizeCPU->Width = offScreenViewport.Width;
		cBufferTextSizeCPU->Height = offScreenViewport.Height;
		updateTextSize(context);
		context->PSSetConstantBuffers(0, 1, &cBufferTextSizeGPU);
		//Horizontal blur
		context->OMSetRenderTargets(1, &intermedHRTV, nullptr);
		context->PSSetShaderResources(0, 1, &intermedSRV);
		context->VSSetShader(screenQuadVS, 0, 0);
		context->PSSetShader(horizontalBlurPS, 0, 0);
		screenQuad->render(context);
		//Vertical blur
		context->OMSetRenderTargets(1, &intermedVRTV, nullptr);
		context->PSSetShaderResources(0, 1, &intermedHSRV);
		context->VSSetShader(screenQuadVS, 0, 0);
		context->PSSetShader(verticalBlurPS, 0, 0);
		screenQuad->render(context);

		// Restore Scene rendertarget dont need depth buffer here 
		context->OMSetRenderTargets(1, tempRT, nullptr);

		// Restore Scene viewport 
		context->RSSetViewports(1, &currentVP);
		context->PSSetShader(textureCopyPS, 0, 0);

		// Copy blurred orb (intermedVSRV) back to Scene frame buffer
		context->PSSetShaderResources(0, 1, &intermedVSRV);
		context->OMSetBlendState(alphaOnBlendState, blendFactor, 0xFFFFFFFF);
		screenQuad->render(context);

		// Create "nullSRV"  to release intermedVSRV shader resource view
		ID3D11ShaderResourceView	*nullSRV[1] = { 0 };
		context->PSSetShaderResources(0, 1, nullSRV);
		context->OMSetRenderTargets(1, &intermedVRTV, nullptr);
		// Restore Scene depth buffer.
		context->OMSetRenderTargets(1, tempRT, tempDS);
		tempRT[0]->Release();
		tempDS->Release();
	}
}




HRESULT BlurUtility::setupBlurRenderTargets(ID3D11Device *deviceIn, int blurWidth, int blurHeight){

	if (!device || !context)
		return E_FAIL;

	cBufferTextSizeCPU->Width = blurWidth;
	cBufferTextSizeCPU->Height = blurHeight;

	updateTextSize(context);

	// Release as we might be resizing
	if (intermedSRV)
		intermedSRV->Release();
	if (intermedRTV)
		intermedRTV->Release();
	if (intermedHSRV)
		intermedHSRV->Release();
	if (intermedHRTV)
		intermedHRTV->Release();
	if (intermedVSRV)
		intermedVSRV->Release();
	if (intermedVRTV)
		intermedVRTV->Release();

	// Setup Render Targets

	HRESULT hr;

	// Setup viewport for offscreen rendering
	offScreenViewport.TopLeftX = 0;
	offScreenViewport.TopLeftY = 0;
	offScreenViewport.Width = blurWidth;
	offScreenViewport.Height = blurHeight;
	offScreenViewport.MinDepth = 0.0f;
	offScreenViewport.MaxDepth = 1.0f;

	// Setup texture description for offscreen rendering
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = blurWidth;
	texDesc.Height = blurHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	ID3D11Texture2D* tex = 0;

	// Setup shader resource view description for offscreen rendering.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;
	viewDesc.Texture2D.MostDetailedMip = 0;
	// Setup render targetview  description for offscreen rendering.
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	//Create offscreeen Textures, ShaderResourceViews and RenderTargetViews
	hr = device->CreateTexture2D(&texDesc, 0, &tex);
	hr = device->CreateShaderResourceView(tex, &viewDesc, &intermedHSRV);
	hr = device->CreateRenderTargetView(tex, &rtvDesc, &intermedHRTV);
	// Cleanup--we only need the views.
	tex->Release();
	hr = device->CreateTexture2D(&texDesc, 0, &tex);
	hr = device->CreateShaderResourceView(tex, &viewDesc, &intermedVSRV);
	hr = device->CreateRenderTargetView(tex, &rtvDesc, &intermedVRTV);
	// Cleanup--we only need the views.
	tex->Release();
	hr = device->CreateTexture2D(&texDesc, 0, &tex);
	hr = device->CreateShaderResourceView(tex, &viewDesc, &intermedSRV);
	hr = device->CreateRenderTargetView(tex, &rtvDesc, &intermedRTV);
	// Cleanup--we only need the views.
	tex->Release();

	// Setup the Depth Stencil buffer and Depth Stencil View (DSV)
	
	// Setup DepthStencilTexture description
	D3D11_TEXTURE2D_DESC		depthStencilDesc;
	depthStencilDesc.Width = blurWidth;
	depthStencilDesc.Height = blurHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = 1; // Multi-sample properties set to 1(no multisample)
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	// Create DepthStencilTexture
	if (SUCCEEDED(hr))
		hr = device->CreateTexture2D(&depthStencilDesc, 0, &tex);

	// Setup DepthStencilView description
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	// Create DepthStencilView
	if (SUCCEEDED(hr))
		hr = device->CreateDepthStencilView(tex, &descDSV, &depthStencilViewBlur);
	// Cleanup--we only need the views.
	tex->Release();

}


BlurUtility::~BlurUtility()
{
	if (cBufferTextSizeCPU)
		_aligned_free(cBufferTextSizeCPU);

	if (cBufferTextSizeGPU)
		cBufferTextSizeGPU->Release();

	if (intermedHSRV)
		intermedHSRV->Release();
	if (intermedHRTV)
		intermedHRTV->Release();
	if (intermedVSRV)
		intermedVSRV->Release();
	if (intermedVRTV)
		intermedVRTV->Release();
	if (intermedSRV)
		intermedSRV->Release();
	if (intermedRTV)
		intermedRTV->Release();
	

	if (depthStencilViewBlur)
		depthStencilViewBlur->Release();
	if (alphaOnBlendState)
		alphaOnBlendState->Release();
	if (screenQuadVS)
		screenQuadVS->Release();
	if (emissivePS)
		emissivePS->Release();
	if (horizontalBlurPS)
		horizontalBlurPS->Release();
	if (verticalBlurPS)
		verticalBlurPS->Release();
	if (textureCopyPS)
		textureCopyPS->Release();
	if (depthCopyPS)
		depthCopyPS->Release();
	if (perPixelLightingVS)
		perPixelLightingVS->Release();

	if (screenQuad)
		delete(screenQuad);
	if (defaultEffect)
		delete(defaultEffect);
}
