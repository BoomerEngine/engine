/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Frame constants
*
***/

#pragma once

//--

#include <math.h>
#include <camera.h>

//--

#define MATERIAL_FLAG_DISABLE_COLOR 1
#define MATERIAL_FLAG_DISABLE_LIGHTING 2
#define MATERIAL_FLAG_DISABLE_TEXTURES 4
#define MATERIAL_FLAG_DISABLE_NORMAL 8
#define MATERIAL_FLAG_DISABLE_VERTEX_COLOR 16
#define MATERIAL_FLAG_DISABLE_OBJECT_COLOR 32
#define MATERIAL_FLAG_DISABLE_VERTEX_MOTION 64
#define MATERIAL_FLAG_DISABLE_MASKING 128

//--

// fundamental frame rendering stuff
descriptor FrameParams
{
	ConstantBuffer
	{
		ivec2 ViewportSize; // size of the frame viewport
		ivec2 ViewportRTSize; // size of the frame render target (may be bigger)
		vec2 InvViewportSize;
		vec2 InvViewportRTSize;

		/*ivec2 TargetSize; // size of the target (compisition) viewport - may be totally different size
		ivec2 TargetRTSize; // size of the composition render target (may be bigger)
		float InvTargetSize;
		float InvTargetRTSize;*/

		uint FrameIndex; // running frame index, every frame new number
		uint MSAASamples; // in case of MSAA rendering this is the number of samples, <= 1 if disabled
		uint MaterialFlags; // material debug flags
		uint _PaddingB;
		
		int ScreenTopY; // "top" (sky facing) row of image, flipped on OpenGL, 
		int ScreenBottomY; // "bottom" (ground facing) row of image, flipped on OpenGL
		int ScreenDeltaY; // delta to "lower" (ground facing) row of image, 1 on normal platforms, -1 on OpenGL
		uint _PaddingC;
		
		uint4 PseudoRandom; // pseudo random numbers - new every frame

		ivec2 DebugMousePos; // position (in Frame pixel coords) of mouse cursor, can be used for debugging shaders
		ivec2 DebugMouseClickPos; // position (in Frame pixel coords) of last mouose click

		float GameTime; // game time counter, ticked only when game is ticked, affected by slomo
		float EngineTime; // engine time, ticked in RT
		float TimeOfDay; // normalized (0-24) time of day
		float DayNightFraction; // normalized (0-1) night/day factor
	}
}

//--

shader Viewport
{
	vec2 CalcScreenPosition(ivec2 pos)
	{
		return pos * InvViewportSize * 2 - 1;
		//return pos * InvViewportSize;
		
	}
	
	vec3 CalcWorldPosition(ivec2 pos, float depth)
	{
		return Camera.CalcWorldPosition(vec3(CalcScreenPosition(pos), depth));
	}
}

//--

shader Frame
{
	bool CheckMaterialDebug(uint flag)
	{
		return 0 != (FrameParams.MaterialFlags & flag);
	}
}

//--