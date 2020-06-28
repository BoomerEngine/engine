/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Lumimance visualisation shader
*
***/

//----

#include "postfx.h"
#include <text_print.h>
#include <frame.h>
#include <camera.h>

//--

descriptor LumVisParams
{
#ifdef VIS_MSAA
	attribute(multisample) 
#endif
	Texture2D SourceColor;
}

export shader PS
{
	// https://www.shadertoy.com/view/ltlSRj
	vec3 fromRedToGreen( float interpolant )
	{
		if( interpolant < 0.5 )
		   return vec3(1.0, 2.0 * interpolant, 0.0); 
		else
			return vec3(2.0 - 2.0 * interpolant, 1.0, 0.0 );
	}


	vec3 fromGreenToBlue( float interpolant )
	{
		if( interpolant < 0.5 )
		   return vec3(0.0, 1.0, 2.0 * interpolant); 
		else
			return vec3(0.0, 2.0 - 2.0 * interpolant, 1.0 );
	}

	vec3 heat5( float interpolant )
	{
		float invertedInterpolant = interpolant;
		if( invertedInterpolant < 0.5 )
		{
			float remappedFirstHalf = 1.0 - 2.0 * invertedInterpolant;
			return fromGreenToBlue( remappedFirstHalf );
		}
		else
		{
			float remappedSecondHalf = 2.0 - 2.0 * invertedInterpolant; 
			return fromRedToGreen( remappedSecondHalf );
		}
	}

	vec3 heat7( float interpolant )
	{
		if( interpolant < 1.0 / 6.0 )
		{
			float firstSegmentInterpolant = 6.0 * interpolant;
			return ( 1.0 - firstSegmentInterpolant ) * vec3(0.0, 0.0, 0.0) + firstSegmentInterpolant * vec3(0.0, 0.0, 1.0);
		}
		else if( interpolant < 5.0 / 6.0 )
		{
			float midInterpolant = 0.25 * ( 6.0 * interpolant - 1.0 );
			return heat5( midInterpolant );
		}
		else
		{
			float lastSegmentInterpolant = 6.0 * interpolant - 5.0; 
			return ( 1.0 - lastSegmentInterpolant ) * vec3(1.0, 0.0, 0.0) + lastSegmentInterpolant * vec3(1.0, 1.0, 1.0);
		}
	}
	
	float ReadLuminance(ivec2 pixelCoord)
	{
#ifdef VIS_MSAA	
		vec3 linearColor = textureLoadSample(SourceColor, pixelCoord, 0).xyz;
#else
		vec3 linearColor = SourceColor[pixelCoord].xyz;
#endif		
		return max(0.001, Luminance(linearColor));
	}

	void main()
	{
		ivec2 pixelCoord = gl_FragCoord.xy;
		
		float lum = ReadLuminance(pixelCoord);
		float lumLog = max(0, log2(lum) + 4);
		
		gl_Target0 = heat7(lumLog / 6.0).xyz1;
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
