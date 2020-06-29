/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Simple copy shader
*
***/

//----

#include <math.h>

//--

descriptor SkyParams
{
    ConstantBuffer
	{
		ivec2 TargetSize;
		vec2 CenterPos;
		float Time;
	}
}

export shader BlitPS
{
	vec3 CalcPlasma(float x, float y, float t)
	{
		float h = 20;
		float fx = x * h - h / 2;
		float fy = y * h - h / 2;

		float v = 0.0;
		v = v + sin((fx + t));
		v = v + sin((fy + t) / 2.0);
		v = v + sin((fx + fy + t) / 2.0);

		fx = fx + sin(t / 3.0) + h / 2;
		fy = fy + cos(t / 2.0) + h / 2;

		v = v + sin(sqrt(fx * fx + fy * fy + 1.0) + t);
		v = v / 2.0;

		vec3 a = vec3(117, 213, 227) / 255.0f;
		vec3 b = vec3(138, 229, 255) / 255.0f;
		vec3 c = vec3(172, 245, 251) / 255.0f;
		vec3 d = vec3(195, 251, 249) / 255.0f;

		vec3 ab = lerp(a, b, 0.5+0.2*sin(PI * v));
		vec3 cd = lerp(c, d, 0.5 + 0.2*cos(PI * v));
		return lerp(ab, cd, 1-y);
	}


	void main()
    {
		vec2 uv = (gl_FragCoord.xy / vec2(TargetSize)) * 3.0;
		gl_Target0 = CalcPlasma(uv.x + CenterPos.x * 0.0005, uv.y, Time).xyz1;
	}
}

export shader BlitVS
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
