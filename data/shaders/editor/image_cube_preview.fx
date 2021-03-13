/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#include <math.h>
#include "image_preview_common.h"

//--

descriptor CubePreviewParams
{
	ConstantBuffer
	{
		mat4 WorldToScreen;
		mat4 LocalToWorld;

		vec4 ColorSelector;
		int MipIndex;
		int sliceIndex;
		int ColorSpace;
		float ColorSpaceScale;
		int ToneMapMode;
	}

	Sampler PreviewTextureSampler;
	attribute(sampler=PreviewTextureSampler) TextureCube PreviewTexture;
}

attribute(packing = vertex) struct CubePreviewVertex
{
	attribute(offset = 0) vec3 pos;
	attribute(offset = 12) vec3 normal;
}

export shader CubeVS
{
	vertex CubePreviewVertex v;

	out vec3 WorldPosition;
	out vec3 WorldNormal;

	//attribute(glflip)
	void main()
	{
		WorldPosition = (LocalToWorld * v.pos.xyz1).xyz;
		WorldNormal = (LocalToWorld * v.normal.xyz0).xyz;

		gl_Position = WorldToScreen * WorldPosition.xyz1;
	}
}

state TwoSided
{
	CullEnabled = false,
	DepthEnabled = true,
	DepthWriteEnabled = true,
}

attribute(state = TwoSided)
export shader CubePS
{
	in vec3 WorldPosition;
	in vec3 WorldNormal;

	void main()
	{
		vec4 color = vec4(1,0,1,1);

#ifdef USE_NORMAL
		vec3 dir = WorldNormal;
#else
		vec3 dir = normalize(WorldPosition);
#endif

		color = textureLod(PreviewTexture, dir, MipIndex);

		ImagePreviewSetup setup;
		setup.colorSpaceScale = ColorSpaceScale;
		setup.colorSpace = ColorSpace;
		setup.toneMapMode = ToneMapMode;
		setup.colorFilter = ColorSelector;

		//color.xyz = 0.5 + 0.5 * dir;

		gl_Target0 = CalcPreviewColor(setup, color);
	}
}

//--