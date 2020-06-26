/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#include <math.h>
#include "canvas.h"

//--

export shader CanvasVS;

export shader CustomCanvasPS
{
	in vec2 CanvasUV;

	void main()
	{
		vec2 c = CanvasUV;
		vec2 z = c;

		float frac = 1.0f;
		for (int i=0; i<512; i = i + 1)
		{
			float dist = length(z);
			if (dist > 2)
			{
				frac = i / 512.0f;
				break;
			}
				
			z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y);
			z = z + c;
		}

		float frac2 = 1.0f - pow(1.0f - frac, 9.0f);
		gl_Target0 = vec4(sin(PI*frac2), cos(PI*frac2), 0, 1);
	}
}

//--
