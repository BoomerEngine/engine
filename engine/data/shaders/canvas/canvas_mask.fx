/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#include "canvas.h"

//--

export shader CanvasVS; // use standard shader

export shader CanvasSimplePS
{
	void main()
	{
		gl_Target0 = vec4(0,0,0,0);
	}
}

//--
