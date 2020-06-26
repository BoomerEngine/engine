/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#pragma once

#include <math.h>
#include <frame.h>
#include <camera.h>
#include "vertex.h"

//--

export shader MaterialPS
{
	void main()
	{
		gl_Target0 = vec4(1,0,1,1);
	}
}

//--