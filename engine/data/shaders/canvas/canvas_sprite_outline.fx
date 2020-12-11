/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Canvas Rendering Pipeline
*
***/

#include "canvas.h"

//--

descriptor OutlineParameters
{
	ConstantBuffer
	{
		uint OutlineColor;
		float OutlineRadius;
		float OutlineRadiusInvFrac;
	}
}

export shader CanvasVS; // use standard shader

export shader CanvasFillPS : CanvasPS
{
	void main()
	{
		// start with vertex color
		vec4 color = ComputeColorAt(CanvasUV);
		float centerAlpha = color.a;
		color *= CanvasColor;

		// calculate and apply line AA
		uint flags = UnpackFlags();
		if (flags & MASK_HAS_FRINGE)
			color.a *= (1.0 - max(0, abs(CanvasUV.y) - CalcLineHalfWidth())) * CanvasUV.x;

		// apply outline
		if (centerAlpha < 1)
		{
			vec2 uvStep = vec2(1) / textureSizeLod(AtlasTexture, 0).xy;

			float maxAlpha = 0.0f;
			int size = max(1, ceil(OutlineRadius));

			for (int y = -size; y <= size; y += 1)
			{
				for (int x = -size; x <= size; x += 1)
				{
					if (x != 0 && y != 0)
					{
						vec2 uv = CanvasUV + vec2(x, y) * uvStep;

						float dist = max(0, length(vec2(x, y)) - 1);
						float distFrac = 1.0f - (dist * OutlineRadiusInvFrac);
						distFrac = pow(distFrac, 0.2);
						maxAlpha = max(maxAlpha, ComputeColorAt(uv).a * distFrac);

						if (maxAlpha > 0.99)
							break;
					}
				}
			}

			vec3 outlineColor = unpackUnorm4x8(OutlineColor).xyz;
			float outlineFactor = saturate(maxAlpha - color.a);
			color.xyz = lerp(color.xyz, outlineColor, outlineFactor);
			color.a = max(color.a, outlineFactor);
		}

		// apply scissor mask to remove all parts outside of clip rect
		color.a *= CalcScissorMask(CanvasPosition);

		// assemble final color with all opacities, apply dither since most of canvas drawing is not color-perfect
		gl_Target0 = ScreenSpaceDither(color);
		gl_Target0.xyz *= gl_Target0.a;
	}
}

//---