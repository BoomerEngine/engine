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
		CanvasRenderStyle style = RenderStyles[CanvasBatchID];
		
		// calcualte the position within the painting area
		vec2 pt = CanvasPaintUV.xy;
		
		//--
		
		// apply shading
		vec4 color = CanvasColor;
		
		// apply gradient
		if (style.ShaderType == 0)
		{			
			// Gradient only
			float d = saturate((CalcRoundRect(pt, style.Extent, style.Radius) + style.Feather*0.5) / style.Feather); 
			color *= ScreenSpaceDither(lerp(style.InnerCol,style.OuterCol,d));
		}
		else if (style.ShaderType == 1)
		{
			// calcualte the uv for textures
			vec2 uv = (style.WrapType & 4) ? CanvasUV : ((pt * style.ExtentInv) + style.Base);
			uv.x = WrapRange(style.WrapType & 1, uv.x, style.UVMin.x, style.UVMax.x);
			uv.y = WrapRange(style.WrapType & 2, uv.y, style.UVMin.y, style.UVMax.y);
			
			// scale uv to fit the placement of the image in the image atlas
			uv = style.UVBias + (uv * style.UVScale);
			
			// multiply with style color that texture can still have
			color *= style.InnerCol.xyzw;
			
			// apply texture
			if (style.TextureType == 1)
				color *= textureLod(AlphaTextureAtlas, float3(uv, style.TextureLayer), 0).xxxx;
			else if (style.TextureType == 2)
				color *= textureLod(ColorTextureAtlas, float3(uv, style.TextureLayer), 0);
		}
		
		// apply the scrissor clip
		gl_Target0 = color * CalcScissorMask(CanvasPosition);
	}
}

//---