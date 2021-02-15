/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

#include <math.h>

descriptor LinearizeDepthParams
{
	ConstantBuffer
	{ 
		uvec2 ScreenSize;
		vec2 LinearizeZ;
	}

	Texture2D DepthTexture;
    attribute(uav, format=r32f) Texture2D LinearizedDepth;
}

export shader GenerateCS
{	
	float CalcLinearizeViewDepth(float screenDepthValue)
	{
		// screenZ = (worldZ * m22 + m23) / worldZ;
		// screenZ = m22 + m23 / worldZ;
		// screenZ - m22 = m23 / worldZ;
		// (screenZ - m22) / m23 = 1 / worldZ;
		// worldZ = m23 / (screenZ - m22);

		return LinearizeZ.y / (screenDepthValue - LinearizeZ.x);
	}

	attribute(local_size_x=8, local_size_y=8)
	void main()
	{
		uvec2 pos = gl_GlobalInvocationID.xy;
		uvec2 readPos = min(pos, ScreenSize - uvec2(1, 1));
		float depth = DepthTexture[readPos].x;
		LinearizedDepth[pos] = CalcLinearizeViewDepth(depth);
	}
}
