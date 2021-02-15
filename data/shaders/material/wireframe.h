/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Wireframe generator
*
***/

#pragma once

//----

export shader MaterialGS
{
	out vec3 TriangleBarycentric;
	out uint ObjectIndex;

	attribute(binding=ObjectIndex) in uint[] ObjectIndexIn;
	
	attribute(input=triangles, output=triangle_strip, max_vertices=3)
	void main()
	{
		TriangleBarycentric = vec3(1,0,0);
        ObjectIndex = ObjectIndexIn[0];
		gl_Position = gl_PositionIn[0];
		EmitVertex();
		
		TriangleBarycentric = vec3(0,1,0);
        ObjectIndex = ObjectIndexIn[1];
		gl_Position = gl_PositionIn[1];
		EmitVertex();
		
		TriangleBarycentric = vec3(0,0,1);
		ObjectIndex = ObjectIndexIn[2];
		gl_Position = gl_PositionIn[2];
		EmitVertex();
		EndPrimitive();
	}
}

export shader MaterialPS
{
	in vec3 TriangleBarycentric;
	
	const float LINE_WIDTH = 0.5;
	
	float CalcEdgeFactor()
	{
		vec3 d = fwidth(TriangleBarycentric);
		vec3 f = step(d * LINE_WIDTH, TriangleBarycentric);
		float fade = saturate(length(d) / 10.0);
		return lerp(min(min(f.x, f.y), f.z), 1, fade);
	}
}
	

//----

