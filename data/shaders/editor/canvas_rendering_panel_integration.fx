/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#include <math.h>
#include <canvas/canvas.h>

//--

sampler BlitSampler
{
	MinFilter=Linear,
	MagFilter=Linear,
	MipFilter=None,
	AddressU=Clamp,
	AddressV=Clamp,
	AddressW=Clamp,
}

descriptor CanvasRenderingPanelIntegrationParams
{
	attribute(sampler=BlitSampler) Texture2D PreviewTexture;
}

export shader CanvasVS; // use standard shader

export shader CustomCanvasPS : CanvasPS
{
	void main()
	{
		vec4 color = textureLod(PreviewTexture, CanvasUV, 0).xyz1; // no alpha
		gl_Target0 = color * CalcScissorMask(CanvasPosition);
	}
}

//--