/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#include <math.h>
#include <canvas/canvas.h>
#include "image_preview_common.h"

//--

descriptor ExtraParams
{
	ConstantBuffer
	{
		vec4 ColorFilter;

		int MipIndex;
		int SliceIndex;
		int ColorSpace;
		float ColorSpaceScale;
		int ToneMapMode;
	}

#ifdef USE_TEXTURE_2D
	Sampler PreviewTextureSampler;
	attribute(sampler=PreviewTextureSampler) Texture2D PreviewTexture;
#elif defined(USE_TEXTURE_ARRAY)
	Sampler PreviewTextureSampler;
	attribute(sampler = PreviewTextureSampler) Texture2DArray PreviewTextureArray;
#endif
}

export shader CanvasVS; // use standard shader

export shader CustomCanvasPS : CanvasPS
{
	void main()
	{
		vec4 color = vec4(1,0,1,1);

		ImagePreviewSetup setup;
		setup.colorSpaceScale = ColorSpaceScale;
		setup.colorSpace = ColorSpace;
		setup.toneMapMode = ToneMapMode;
		setup.colorFilter = ColorFilter;

#ifdef USE_TEXTURE_2D
		color = CalcPreviewColor(setup, textureLod(PreviewTexture, CanvasUV, MipIndex));
#elif defined(USE_TEXTURE_ARRAY)
		color = CalcPreviewColor(setup, textureLod(PreviewTextureArray, vec3(CanvasUV.xy, SliceIndex), MipIndex));
#endif

		gl_Target0 = color * CalcScissorMask(CanvasPosition);
	}
}

//--