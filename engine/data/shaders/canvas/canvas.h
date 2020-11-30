/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Common canvas stuff
*
***/

//--

attribute(packing=vertex) struct CanvasVertex
{
    attribute(offset=0) vec2 pos;
    attribute(offset=8) vec2 uv;
	attribute(offset=16) vec2 paintUV;
	attribute(offset=24) vec2 clipUV;
    attribute(offset=32, format=rgba8) vec4 color;
	attribute(offset=36) float batchID;
}

sampler CanvasSampler
{
	MagFilter = Linear,
	MinFilter = Linear,
	MipFilter = None,
	AddressU = Clamp,
	AddressV = Clamp,
	AddressW = Clamp,
	MaxLod = 0,
}

setup CanvasDefault
{
	DepthEnabled = true,
	DepthWriteEnabled = false,
	StencilEnabled = true,
}

struct CanvasRenderStyle
{
	vec4 InnerCol;
	vec4 OuterCol;
	
	vec2 Base;
	vec2 _Padding4;
	
	vec2 Extent;
	vec2 ExtentInv;
	
	vec2 UVBias;
	vec2 UVScale;
	vec2 UVMin;
	vec2 UVMax;
	
	float FeatherHalf;
	float FeatherInv;
	float Radius;
	float Feather;
	
	uint TextureType;
	uint TextureLayer;
	uint ShaderType;
	uint WrapType;
}
	
descriptor CanvasViewportParams
{
	ConstantBuffer
	{
		mat4 CanvasToScreen;
	}

	attribute(layout=CanvasRenderStyle) Buffer RenderStyles;

	sampler LocalSampler;
	attribute(sampler=LocalSampler) Texture2DArray AlphaTextureAtlas;
	attribute(sampler=CanvasSampler) Texture2DArray ColorTextureAtlas;
}

//--

// common vertex shader
shader CanvasVS
{
	vertex CanvasVertex v;

	out vec2 CanvasPosition;
	out vec2 CanvasUV;
	out vec2 CanvasPaintUV;
	out vec2 CanvasClipUV;
	out vec4 CanvasColor;
	out float CanvasBatchID;

	void main()
	{
		CanvasPosition = v.pos;
		CanvasUV = v.uv;
		CanvasPaintUV = v.paintUV;
		CanvasClipUV = v.clipUV;
		CanvasColor = v.color;
		CanvasBatchID = v.batchID;

		gl_Position = v.pos.xy01 * CanvasToScreen;
//#if FLIP_Y
//		gl_Position.y = -gl_Position.y;
//#endif
	}
}

//----

// common pixel shader
attribute(setup=CanvasDefault)
shader CanvasPS
{
	in vec2 CanvasPosition;
	in vec2 CanvasUV;
	in vec2 CanvasPaintUV;
	in vec2 CanvasClipUV;
	in vec4 CanvasColor;
	in float CanvasBatchID;

	float CalcRoundRect(vec2 pt, vec2 ext, float rad)
	{
		vec2 ext2 = ext - rad.xx;
		vec2 d = abs(pt) - ext2;
		return min(max(d.x,d.y),0.0) + length(max(d,vec2(0))) - rad;
	}

	float CalcScissorMask(vec2 pos)
	{
		//return 1.0f;

		//vec2 sc = (1.0 - abs(CanvasClipUV.xy)) * 32768.0; // mask that is > 0 inside < 0 outside
		vec2 sc = 1.0 - abs(CanvasClipUV.xy); // mask that is > 0 inside < 0 outside

		//return saturate(sc.x) * saturate(sc.y); // bring back to 1 inside, 0 outside + merge X and Y
		return all(sc.xy >= 0.0);
	}

	/*float CalcStrokeMask()
	{
		float strokeMul = RenderStyles[CanvasBatchID].StrokeMult;
		return min(1.0, (1.0-abs(CanvasUV.x*2.0-1))*strokeMul) * min(1.0, CanvasUV.y);
	}*/

	float WrapRange(bool wrap, float x, float xmin, float xmax)
	{
		if (wrap)
		{
			float range = xmax - xmin;
			return xmin + (x - xmin) % range;
		}
		else
		{
			return max(xmin, min(xmax, x));
		}
	}

	// http://alex.vlachos.com/graphics/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf
	vec4 ScreenSpaceDither(vec4 linearColor)
	{
		float ditherScale = 2.0 / 255.0;
		float ditherOffset = 1.0 / 255.0;

		float3 dither = dot(vec2(171.0, 231.0), gl_FragCoord.xy).xxx;
		dither = frac(dither / float3(103.0, 71.0, 97.0)) * ditherScale - ditherOffset;

		//linearColor.xyz += sqrt(linearColor.xyz) * dither * 2;
		return linearColor;
	}

}

//----

