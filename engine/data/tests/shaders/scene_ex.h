/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

#pragma once

#include "scene.h"

//--

shader SceneGS
{
	in vec3[] WorldPosition;
	in vec3[] WorldNormal;
	in vec2[] UV;

	attribute(binding=WorldPosition) out vec3 WorldPositionOut;
	attribute(binding=WorldNormal) out vec3 WorldNormalOut;
	attribute(binding=UV) out vec2 UVOut;

	attribute(input=triangles, output=triangle_strip, max_vertices=3, invocations=6)
	void main()
	{
		for (int i=0; i<3; i += 1)
		{
			WorldPositionOut = WorldPosition[i];
			WorldNormalOut = WorldNormal[i];
			UVOut = UV[i];

			gl_Position = Camera[gl_InvocationID].WorldToScreen * WorldPosition[i].xyz1;
			gl_Layer = gl_InvocationID;
			EmitVertex();
		}

		EndPrimitive();
	}
}

//--