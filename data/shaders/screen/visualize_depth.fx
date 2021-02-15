/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Simple copy shader
*
***/

//----

#include <math.h>
#include <text_print.h>
#include <frame.h>
#include <camera.h>

//--

descriptor DepthVisParams
{
#ifdef VIS_MSAA
	attribute(multisample)
#endif
	Texture2D SourceDepth;
}

export shader PS
{
	float ReadDepth(ivec2 pos)
	{
#ifdef VIS_MSAA	
		return textureLoadSample(SourceDepth, pos, 0).x
#else
		return SourceDepth[pos].x;
#endif
	}
	
	void main()
    {
		ivec2 pixelCoord = gl_FragCoord.xy;
		float depth = ReadDepth(pixelCoord);


		// default gradient		
		float visDepth = pow(depth, 300.0f);
		gl_Target0 = 0.25f + 0.75 * frac(visDepth).xxx1;

		// depth at mouse position
		float depthAtMouse = ReadDepth(DebugMousePos);
		
		float highlightRange = 1.0001;
		float highlightDepthMin = depthAtMouse / highlightRange;
		float highlightDepthMax = depthAtMouse * highlightRange;
		
		// range
		if (DebugMousePos.x != -1 && DebugMousePos.y != -1 && depthAtMouse < 1.0)
		{
			if (depth >= highlightDepthMin && depth <= highlightDepthMax)
			{
				gl_Target0.xyz *= vec3(1,0.8,0.8);
			}
		}
		
		// world grid
		if (depth < 1.0f)
		{
			vec3 pos = Viewport.CalcWorldPosition(pixelCoord, depth);
			
			{
				ivec3 tile = pos;
				int coord = (tile.x&1) ^ (tile.y&1) ^ (tile.z&1);
				if (coord)
					gl_Target0.xyz = saturate(gl_Target0.xyz * 0.8);
			}

			
			float grid = 1 - dot(1 - saturate(abs(pos - round(pos))*50), vec3(1));
			
			pos *= 2;
			grid *= 1 - (dot(1 - saturate(abs(pos - round(pos))*50), vec3(1)) * 0.5);
			
			pos *= 2;
			grid *= 1 - (dot(1 - saturate(abs(pos - round(pos))*50), vec3(1)) * 0.25);
			
			pos *= 2;
			grid *= 1 - (dot(1 - saturate(abs(pos - round(pos))*50), vec3(1)) * 0.25);
			
			gl_Target0 *= saturate(grid);
		}

		// info overlay
		if (DebugMousePos.x != -1 && DebugMousePos.y != -1)
		{
			 /*if (depth > highlightDepthMax && depth < 1.0)
			{
				float linearDistanceMouse = Camera.CalcLinearizeViewDepth(depthAtMouse);
				float linearDistanceCur = Camera.CalcLinearizeViewDepth(depth);
				float distanceBehind = max(0, linearDistanceCur - linearDistanceMouse);
				
				float fog = min(0.4, 1 - exp(-distanceBehind * 0.1));
				gl_Target0.xyz = lerp(gl_Target0.xyz, vec3(1,0,0), fog);
			}*/
			
			if (all(pixelCoord == DebugMousePos))
				gl_Target0 = vec4(1, 0, 0, 1);

			if (pixelCoord.x == DebugMousePos.x || pixelCoord.y == DebugMousePos.y)
				gl_Target0.xyz *= 0.9;
				
			if (depthAtMouse > 0.0 && depthAtMouse < 1.0)
			{
				int value = depthAtMouse * 10000000;
				int linValue = Camera.CalcLinearizeViewDepth(depthAtMouse) * 100;
				
				if (TextPrinter.DisplayValue(pixelCoord, DebugMousePos + ivec2(30,0), value, 4, 1, 0))
					gl_Target0 = vec4(1, 0, 0, 1);
					
				if (TextPrinter.DisplayValue(pixelCoord, DebugMousePos + ivec2(30,40), linValue, 4, 1, 0))
					gl_Target0 = vec4(0, 1, 0, 1);
			}
		}
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
