
//
// Grass effect - Modified a fur technique
//

// Ensure matrices are row-major
#pragma pack_matrix(row_major)


//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------



cbuffer modelCBuffer : register(b0) {

	float4x4			worldMatrix;
	float4x4			worldITMatrix; // Correctly transform normals to world space
};
cbuffer cameraCbuffer : register(b1) {
	float4x4			viewMatrix;
	float4x4			projMatrix;
	float4				eyePos;
}
cbuffer lightCBuffer : register(b2) {
	float4				lightVec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				lightAmbient;
	float4				lightDiffuse;
	float4				lightSpecular;
};
cbuffer sceneCBuffer : register(b3) {
	float4				windDir;
	float				Time;
	float				grassHeight;
};

//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
Texture2D diffuseTexture : register(t0);
Texture2D grassAlpha: register(t1);
SamplerState anisotropicSampler : register(s0);




//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------

// Input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct FragmentInputPacket {

	// Vertex in world coords
	float3				posW			: POSITION;
	// Normal in world coords
	float3				normalW			: NORMAL;
	float4				matDiffuse		: DIFFUSE; // a represents alpha.
	float4				matSpecular		: SPECULAR; // a represents specular power. 
	float2				texCoord		: TEXCOORD;
	float4				posH			: SV_POSITION;
};


struct FragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};


//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) {

	FragmentOutputPacket outputFragment;

	float maxGrassDist = 5.0f;
	float grassFadeRange = 20.f;
	float distance = length(eyePos.xyz - v.posW);

	if (grassHeight > 0.0)
	{
		// Improve rendering speed by clipping if grass is distant or below water level
		clip(distance > maxGrassDist + grassFadeRange ? -1 : 1);
		clip(v.posW.y < -0.09 ? -1 : 1);
	}

	float grassFadeDist = saturate((distance - maxGrassDist) / grassFadeRange);
	float tileRepeat = 30;
	float3 baseColour = v.matDiffuse.xyz * diffuseTexture.Sample(anisotropicSampler, v.texCoord) * 3.0;
	float alpha = 1.0;
	float3 N = normalize(v.normalW);
	float mudTop = 0.1;
	float fadeToMud = saturate(abs((v.posW.y - mudTop) * 6.0f));// Used to lerp from grass to mud colour
	float3 brownMudColour = float3(1.8, 0.3, 0.0);

	// Turn grass colour into brown mud colour below water level
	if (v.posW.y < mudTop)
		baseColour = lerp(baseColour, brownMudColour * baseColour, fadeToMud);

	// Ambient light
	float3 colour = baseColour * lightAmbient;

	// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
	float3 lightDir = -lightVec.xyz; // Directional light
	if (lightVec.w == 1.0) lightDir = lightVec.xyz - v.posW; // Positional light
	lightDir = normalize(lightDir);
	colour += max(dot(lightDir, N), 0.0f) * baseColour * lightDiffuse;

	//Render grass
	if (grassHeight > 0.0)
	{
		alpha = grassAlpha.Sample(anisotropicSampler, v.texCoord * tileRepeat).a;
		// Reduce alpha and increase illumination for tips of grass
		colour *= (alpha + grassHeight) * 2.2;// *(1 - alpha) * 3;
		alpha = (alpha - grassHeight * 40);
		// Fade out the grass alpha with distance and ground height
		if (v.posW.y < mudTop)
			alpha = lerp(alpha, 0.0, fadeToMud);
		alpha = lerp(alpha, 0.0, grassFadeDist);
	}


	// Add fog - desaturate (turn grey) with increasing distance from camera
	float maxFogDist = 100.0f;
	float grey = float3(0.5, 0.5, 0.5);
	colour = lerp(colour, (grey + colour) / 2.0f, saturate(distance / maxFogDist));

	outputFragment.fragmentColour = float4(colour, alpha);

	return outputFragment;

}
