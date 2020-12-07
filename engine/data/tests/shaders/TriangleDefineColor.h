/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

#include "common.h"

#ifndef TRIANGLE_COLOR
	#define TRIANGLE_COLOR vec3(1,0,1)
#endif

export shader TestVS
{
	vertex Vertex2D v;

	attribute(glflip)
	void main()
	{
		gl_Position = v.pos.xy01;
	}
}

export shader TestPS 
{
	void main()
	{
		gl_Target0 = TRIANGLE_COLOR.xyz1;
	}
}
