/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Simple checker shader
*
***/

//----

#include "postfx.h"

//----

descriptor CheckerParams
{
	ConstantBuffer
	{
		vec4 ColorA;
		vec4 ColorB;
		uint Size;
	}
}

//----

export shader CheckerPS
	{
		void main()
		{
			uvec2 sourcePixel = uvec2(gl_FragCoord.xy) / Size;

			if ((sourcePixel.x ^ sourcePixel.y) & 1)
				gl_Target0 = ColorA;
			else
				gl_Target0 = ColorB;
		}
	}

		export shader CheckerVS
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