/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#include "canvas.h"

//--

export shader CanvasVS; // use standard shader

export shader CanvasFillPS : CanvasPS
{

 	void main()
	{
		// start with vertex color
		vec4 color = CanvasColor * ComputeColorAt(CanvasUV);

		// calculate and apply line AA
		uint flags = UnpackFlags();
		if (flags & MASK_HAS_FRINGE)
			color.a *= (1.0 - max(0, abs(CanvasUV.y) - CalcLineHalfWidth())) * CanvasUV.x;

		// apply scissor mask to remove all parts outside of clip rect
		color.a *= CalcScissorMask(CanvasPosition);

		// assemble final color with all opacities, apply dither since most of canvas drawing is not color-perfect
		gl_Target0 = ScreenSpaceDither(color);
		gl_Target0.xyz *= gl_Target0.a;
	}
}

//---