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
    attribute(offset=16) vec2 clipUV;
    attribute(offset=24, format=rgba8) vec4 color;
    attribute(offset=28, format=rg16ui) uvec2 attributesPacked; // index + flags
    attribute(offset=32, format=rg16ui) uvec2 imageInfoPacked; // entry index + page index
}

struct CanvasAttributes
{
	vec2 base;
	vec2 extent;
	uint innerColor;
	uint outerColor;
	float radius;
	float feather;
	float lineWidth;
	uint _paddng0;
};

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

state CanvasDefault
{
	DepthEnabled = false,
	DepthWriteEnabled = false,
	PrimitiveTopology = TriangleList,
	//FillMode = Line,
}

struct CanvasImageEntry
{
	vec2 uvMin;
	vec2 uvMax;
	vec2 uvScale;
	vec2 uvInvScale;
}
	
descriptor CanvasDesc
{
	ConstantBuffer
	{
		mat4 CanvasToScreen;

		uint CanvasWidth;
		uint CanvasHeight;
		uint _Padding0;
		uint _Padding1;
	}

	attribute(layout=CanvasAttributes) Buffer Attributes;	
	attribute(layout=CanvasImageEntry) Buffer AtlasImageEntries;

	attribute(sampler=CanvasSampler) Texture2DArray AtlasTexture;
	attribute(sampler=CanvasSampler) Texture2DArray GlyphTexture;
}

//--

// common vertex shader
shader CanvasVS
{
	vertex CanvasVertex v;

	out vec2 CanvasPosition;
	out vec2 CanvasUV;
	out vec2 CanvasClipUV;
	out vec4 CanvasColor;

	out float CanvasAttributesIndex;
	out float CanvasAttributesFlags;
	out float CanvasImageIndex;
	out float CanvasImagePage;
	
	attribute(glflip)
	void main()
	{
		CanvasPosition = v.pos;
		CanvasUV = v.uv;
		CanvasClipUV = v.clipUV;
		CanvasColor = v.color;

		CanvasAttributesIndex = v.attributesPacked.x;
		CanvasAttributesFlags = v.attributesPacked.y;
		CanvasImageIndex = v.imageInfoPacked.x;
		CanvasImagePage = v.imageInfoPacked.y;

		CanvasAttributesIndex = v.attributesPacked.x;
		CanvasAttributesFlags = v.attributesPacked.y;
		CanvasImageIndex = v.imageInfoPacked.x;
		CanvasImagePage = v.imageInfoPacked.y;

		gl_Position = v.pos.xy01 * CanvasToScreen;
	}
}

//----

// common pixel shader
attribute(state=CanvasDefault)
shader CanvasPS
{
	const uint MASK_FILL = 1; // we are a fill
	const uint MASK_STROKE = 2; // we are a stroke
	const uint MASK_GLYPH = 4; // we are a glyph
	const uint MASK_HAS_IMAGE = 8; // we have image
	const uint MASK_HAS_WRAP_U = 16; // do U wrapping
	const uint MASK_HAS_WRAP_V = 32; // do V wrapping
	const uint MASK_HAS_FRINGE = 64; // we have an extra fringe extrusion
	const uint MASK_IS_CONVEX = 128; // what we are rendering is convex

	in vec2 CanvasPosition;
	in vec2 CanvasUV;
	in vec2 CanvasClipUV;
	in vec4 CanvasColor;
	in float CanvasParam;

	//attribute(flat) in float CanvasAttributesPacked;
	//attribute(flat) in float CanvasImageEntryPacked;

	attribute(flat) in float CanvasAttributesIndex;
	attribute(flat) in float CanvasAttributesFlags;
	attribute(flat) in float CanvasImageIndex;
	attribute(flat) in float CanvasImagePage;

	float CalcScissorMask(vec2 pos)
	{
		vec2 sc = 1.0 - abs(CanvasClipUV.xy); // mask that is > 0 inside < 0 outside
		//return saturate(sc.x) * saturate(sc.y); // bring back to 1 inside, 0 outside + merge X and Y
		return all(sc.xy >= 0.0);
	}

	/*float CalcStrokeMask()
	{
		float strokeMul = RenderStyles[CanvasBatchID].StrokeMult;
		return min(1.0, (1.0-abs(CanvasUV.x*2.0-1))*strokeMul) * min(1.0, CanvasUV.y);
	}*/

	float WrapRange(bool wrap, float v, float vmin, float vmax, float vrange, float vinvrange)
	{
		if (wrap)
			return vmin + frac(v * vinvrange) * vrange;
		else
			return clamp(v, vmin, vmax);
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

	// calculate rounded rect SDF
	float CalcRoundRect(vec2 pt, vec2 ext, float rad)
	{
		vec2 ext2 = ext - rad.xx;
		vec2 d = abs(pt) - ext2;
		return min(max(d.x, d.y), 0.0) + length(max(d, vec2(0))) - rad;
	}

	// calculate line width
	float CalcLineHalfWidth()
	{
		//uint idx = floatBitsToUint(CanvasAttributesIndex);
		uint idx = (uint)CanvasAttributesIndex;
		if (idx > 0)
			return Attributes[idx - 1].lineWidth * 0.5f;
		else
			return 0.5f;
	}

	// calculate color for a style
	vec4 CalcStyleColor(uint idx, vec2 pt)
	{
		CanvasAttributes style = Attributes[idx - 1];

		float r = CalcRoundRect(pt, style.extent, style.radius);
		float d = saturate((r + style.feather * 0.5) / style.feather);
		return lerp(unpackUnorm4x8(style.innerColor), unpackUnorm4x8(style.outerColor), d);
	}

	// calculate UV for image sampling
	vec2 CalcImageUV(vec2 pt, uint imageIndex, bool wrapU, bool wrapV, out float alpha)
	{
		vec2 uvMin = AtlasImageEntries[imageIndex].uvMin;
		vec2 uvMax = AtlasImageEntries[imageIndex].uvMax;
		vec2 uvScale = AtlasImageEntries[imageIndex].uvScale;
		vec2 uvInvScale = AtlasImageEntries[imageIndex].uvInvScale;

		if (wrapU)
		{
			pt.x = uvMin.x + frac((pt.x - uvMin.x) * uvInvScale.x) * uvScale.x;
		}
		else if (pt.x < uvMin.x)
		{
			pt.x = uvMin.x;
			alpha = 0;
		}
		else if (pt.x > uvMax.x)
		{
			pt.x = uvMax.x;
			alpha = 0;
		}

		if (wrapV)
		{
			pt.y = uvMin.y + frac((pt.y - uvMin.y) * uvInvScale.y) * uvScale.y;
		}
		else if (pt.y < uvMin.y)
		{
			pt.y = uvMin.y;
			alpha = 0;
		}
		else if (pt.y > uvMax.y)
		{
			pt.y = uvMax.y;
			alpha = 0;
		}

		return pt;
	}

	// calculate image color
	vec4 CalcImageColor(vec2 uv, uint pageIndex)
	{
		return textureLod(AtlasTexture, vec3(uv, pageIndex), 0);
	}

	// calculate glyph color
	vec4 CalcGlyphColor(vec2 uv, uint pageIndex)
	{
		float alpha = textureLod(GlyphTexture, vec3(uv, pageIndex), 0).r;
		return vec4(1, 1, 1, alpha);
	}

	// unpack flags
	uint UnpackFlags()
	{
		return CanvasAttributesFlags;
	}

	// compute basic color at given UV
	vec4 ComputeColorAt(vec2 uv)
	{
		uint flags = UnpackFlags();
		vec4 color = vec4(1, 1, 1, 1);

		// apply color style, only if we have attributes
		if (CanvasAttributesIndex)
			color *= CalcStyleColor(CanvasAttributesIndex, uv);

		// apply image
		if (flags & MASK_HAS_IMAGE) {
			float alpha = 1.0;
			vec2 imageUV = CalcImageUV(uv, CanvasImageIndex, flags & MASK_HAS_WRAP_U, flags & MASK_HAS_WRAP_V, alpha);
			color *= CalcImageColor(imageUV, CanvasImagePage) * alpha;
		}
		// apply glyph
		else if (flags & MASK_GLYPH) {
			color *= CalcGlyphColor(uv, CanvasImagePage);
		}

		return color;
	}

}

//----

