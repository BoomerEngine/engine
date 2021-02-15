/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

#include <math.h>
#include <frame.h>
#include <camera.h>
#include <shadows.h>

descriptor ShadowMaskParams
{
	ConstantBuffer
	{ 
		uvec2 ScreenSize;
	}

	Texture2D DepthTexture;
    attribute(uav, format=rgba8) Texture2D ShadowMask; // R=mask
}

export shader GenerateCS
{	
	attribute(local_size_x=8, local_size_y=8)
	void main()
	{
		uvec2 pos = gl_GlobalInvocationID.xy;
		if (all(pos < ScreenSize))
		{
			float depth = DepthTexture[pos].x;
			vec3 worldPos = Viewport.CalcWorldPosition(pos, depth);
			ShadowMask[pos] = Cascades.ComputeShadowsAtPoint(worldPos, pos).x000;
		}
	}
}
