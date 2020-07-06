/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Shadow computation bits and pieces
*
***/

#pragma once

#include "math.h"

struct GlobalCascadesSetup
{
	float4x4 ShadowTransform;
	float4 ShadowOffsetsX;
	float4 ShadowOffsetsY;
	float4 ShadowHalfSizes;
	float4[4] ShadowParams;
	float4 ShadowPoissonOffsetAndBias;
	float4 ShadowTextureSize;
	float4 ShadowFadeScales;
	float4 ShadowDepthRanges;
	uint ShadowQuality;
};

descriptor ShadowParams
{
	ConstantBuffer
	{
		GlobalCascadesSetup CascadeData;
	};	

	attribute(depth) Texture2DArray CascadeShadowMap;
};
