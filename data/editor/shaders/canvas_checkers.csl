/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#include <math.h>
#include <canvas/canvas.h>

//--

descriptor CavasCheckerData
{
	ConstantBuffer
	{
		ivec4 PixelRegionSize;
		vec4 ColorA;
		vec4 ColorB;
	}
}

export shader CanvasVS; // use standard shader

export shader CustomCanvasPS : CanvasPS
{
	vec3 GridColor(int x, int y, int gridSize)
	{
		x /= gridSize;
		y /= gridSize;

		vec3 color = ColorB.xyz;

		if ((x^y) & 1)
			color = ColorA.xyz;

		float contrast = saturate(log2((float)gridSize) / 5.0);
		color = lerp(color, vec3(0.5,0.5,0.5), contrast);

		return color;
	}

	void main()
	{
		vec4 color = float4(0,0,0,0);

		int x = (int)floor(CanvasUV.x);
		int y = (int)floor(CanvasUV.y);

		if (x >= PixelRegionSize.x && x < PixelRegionSize.z && y >= PixelRegionSize.y && y < PixelRegionSize.w)
		{
			color.w = 1;

			float scale = ddx(CanvasUV.x);
			if (scale > 0.0f)
			{
				const float MinPixelSize = 4.0f;

				int gridSize = 1;
				float pixelSize = 1.0f / scale;

				while ((pixelSize * gridSize) < MinPixelSize)
					gridSize *= 2;

				vec3 thisGrid = GridColor(x, y, gridSize);
				vec3 nextGrid = GridColor(x, y, gridSize*2);

				float frac = saturate((pixelSize * gridSize) / MinPixelSize - 1.0f);
				frac = saturate(0.5 + (frac - 0.5) * 2);

				color.xyz = thisGrid;//lerp(nextGrid, thisGrid, frac);
			}
		}
		
		gl_Target0 = color * CalcScissorMask(CanvasPosition);
	}
}

//----
