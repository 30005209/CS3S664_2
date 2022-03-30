
// Depth texture copy pixel shader
cbuffer DepthCBuffer : register(b0) {
	int			Width;
	int			Height;
};

// input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct fragmentInputPacket {

	float2				texCoord	: TEXCOORD;
	float4				posH		: SV_POSITION;
};


struct fragmentOutputPacket {

	float4			fragmentColour : SV_TARGET;
	float			fragmentDepth : SV_DEPTH;
};

//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
Texture2DMS  <float>depthTexture: register(t0);
//Texture2D  depthTexture: register(t0);
//SamplerState linearSampler : register(s0);

fragmentOutputPacket main(fragmentInputPacket inputFragment) {

	fragmentOutputPacket outputFragment;

	//float zBufDepth = depthTexture.Sample(linearSampler, inputFragment.texCoord);
	float zBufDepth = depthTexture.Load(int4(inputFragment.texCoord.x * Width, inputFragment.texCoord.y * Height, 0, 0), 0).r;
	outputFragment.fragmentColour = float4(0, 0, 0, 0);// colour;// float4(colour.rgb, (colour.r + colour.g + colour.b) / 3);
	outputFragment.fragmentDepth =zBufDepth;

	return outputFragment;
}
