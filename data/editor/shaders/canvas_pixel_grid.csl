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

descriptor CanvasPixelGridParams
{
	ConstantBuffer
	{
		ivec4 PixelRegionSize;
	}
}

export shader CanvasVS; // use standard shader

export shader CustomCanvasPS : CanvasPS
{
	void main()
	{
		vec4 color = float4(0,0,0,0);

		int x0 = (int)floor(CanvasUV.x);
		int y0 = (int)floor(CanvasUV.y);

		vec2 du = abs(ddx(CanvasUV));
		vec2 dv = abs(ddy(CanvasUV));

		int x1 = (int)floor(CanvasUV.x - du.x);
		int y1 = (int)floor(CanvasUV.y - dv.y);

		if (y0 >= PixelRegionSize.y && y0 <= PixelRegionSize.w)
		{
			if (x0 >= PixelRegionSize.x && x1 < PixelRegionSize.z)
			{
				if (y0 != y1)
					color = CanvasColor;
			}
		}

		if (x0 >= PixelRegionSize.x && x0 <= PixelRegionSize.z)
		{
			if (y0 >= PixelRegionSize.y && y1 < PixelRegionSize.w)
			{
				if (x0 != x1)
					color = CanvasColor;
			}
		}

		gl_Target0 = color * CalcScissorMask(CanvasPosition);
	}
}

//--
