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

sampler BlitSampler
{
	MinFilter=Linear,
	MagFilter=Linear,
	MipFilter=None,
	AddressU=Clamp,
	AddressV=Clamp,
	AddressW=Clamp,
}

state BlitQuadState
{
	PrimitiveTopology = TriangleStrip,
}

descriptor BlitParams
{
    ConstantBuffer
	{
		ivec2 TargetPixelOffset;
		vec2 TargetInvPixelScale;

		vec2 SourceOffset;
		vec2 SourceExtents;

		float Gamma;
		float Padding;
	}
	
    attribute(sampler=BlitSampler) Texture2D Source;
}

export shader BlitPS
{
	void main()
    {
		vec2 sourcePixel = saturate((gl_FragCoord.xy - TargetPixelOffset.xy) * TargetInvPixelScale.xy);
		vec2 sourceUV = SourceOffset + (SourceExtents * sourcePixel);

		vec4 val = textureLod(Source, sourceUV, 0);

		gl_Target0 = pow(val.xyz1, Gamma);
		//gl_Target0 = sourceUV.xy01;
	}
}

attribute(state=BlitQuadState)
export shader BlitVS
{
	attribute(glflip)
	void main()
	{
		gl_Position.x = (gl_VertexID & 1) ? -1.0f : 1.0f;
		gl_Position.y = (gl_VertexID & 2) ? -1.0f : 1.0f;	
		gl_Position.z = 0.5f;
		gl_Position.w = 1.0f;
	}
}

//----
