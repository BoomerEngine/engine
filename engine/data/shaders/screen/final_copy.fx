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

descriptor BlitParams
{
    ConstantBuffer
	{
		ivec4 TargetOffsetSize;
		vec2 TargetToSourceScale;
		vec2 SourceInvRTSize;
		float Gamma;
	}
	
    Texture2D Source;
}

export shader BlitPS
{
	void main()
    {
		vec2 sourcePixel = (gl_FragCoord.xy - TargetOffsetSize.xy) * TargetToSourceScale.xy; // in pixels
		vec2 sourceUV = (sourcePixel + 0.0f) * SourceInvRTSize;

		vec4 val = textureLod(Source, sourceUV, 0);

		gl_Target0 = pow(val.xyz1, Gamma);
		//gl_Target0 = sourceUV.xy01;
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
