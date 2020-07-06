/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Simple copy shader
*
***/

//----

#include <math.h>
#include <text_print.h>
#include <frame.h>
#include <camera.h>

//--

descriptor LinearDepthVisParams
{
#ifdef VIS_MSAA
	attribute(multisample)
#endif
	Texture2D SourceDepth;
}

export shader PS
{
	float ReadDepth(ivec2 pos)
	{
#ifdef VIS_MSAA	
		return textureLoadSample(SourceDepth, pos, 0).x;
#else
		return SourceDepth[pos].x;
#endif
	}
	
	void main()
    {
		ivec2 pixelCoord = gl_FragCoord.xy;
		float depth = ReadDepth(pixelCoord);

		gl_Target0.x = frac(depth);
		gl_Target0.y = frac(depth*0.1);
		gl_Target0.z = frac(depth*0.01);
		gl_Target0.w = 1.0f;

		if (DebugMousePos.x != -1 && DebugMousePos.y != -1)
		{
			float depthAtMouse = ReadDepth(DebugMousePos);

			if (all(pixelCoord == DebugMousePos))
				gl_Target0 = vec4(1, 0, 0, 1);

			if (pixelCoord.x == DebugMousePos.x || pixelCoord.y == DebugMousePos.y)
				gl_Target0.xyz *= 0.9;
				
			if (depthAtMouse > 0.0 && depthAtMouse < 1.0)
			{
				int value = (int)(depthAtMouse * 100);
				
				if (TextPrinter.DisplayValue(pixelCoord, DebugMousePos + ivec2(30,0), value, 4, 1, 0))
					gl_Target0 = vec4(1, 0, 0, 1);
			}
		}
	}
}

export shader VS
{
	void main()
	{
		gl_Position.x = (gl_VertexID & 1) ? -1.0f : 1.0f;
		gl_Position.y = (gl_VertexID & 2) ? -1.0f : 1.0f;	
		gl_Position.z = 0.5f;
		gl_Position.w = 1.0f;
	}
}

//----
