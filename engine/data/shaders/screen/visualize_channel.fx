/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Lumimance visualisation shader
*
***/

//----

#include <math.h>
#include <text_print.h>
#include <frame.h>
#include <camera.h>

//--

descriptor ChannelVisParams
{
	ConstantBuffer
	{
		vec4 ColorDot;
	}

#ifdef VIS_MSAA
	attribute(multisample) 
#endif
	Texture2D SourceColor;
}

export shader PS
{
	vec4 ReadValue(ivec2 pixelCoord)
	{
#ifdef VIS_MSAA	
		return textureLoadSample(SourceColor, pixelCoord, 0);
#else
		return SourceColor[pixelCoord];
#endif		
	}

	void main()
	{
		ivec2 pixelCoord = gl_FragCoord.xy;
		vec4 value = ReadValue(pixelCoord);

		float dot = dot(value, ColorDot);
		gl_Target0 = dot.xxx1;
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
