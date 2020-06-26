/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Loading splash screen
*
***/

//--

descriptor SpashScreenParams
{
	ConstantBuffer
	{
		vec2 SourceInvRTSize;
		float Time;
		float Fade;
	}
}

export shader PS
{
	void main()
    {
		ivec2 sourcePixel = gl_FragCoord.xy;
		vec2 sourceUV = (sourcePixel + 0.5f) * SourceInvRTSize;
		gl_Target0 = sourceUV.xy01;
	}
}

export shader VS
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
