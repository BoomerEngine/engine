/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Debug Geometry Rendering Pipeline
*
***/

#include <math.h>
#include <selection.h>

struct DebugAtlasEntry
{
	vec2 uvMin;
	vec2 uvScale;
	uint pageIndex;
	uint _padding0;
};

attribute(packing=vertex) struct DebugVertex
{
    float3 pos;
    float size;
    uint flags;
    float2 uv;
    uint selectableObject;
    uint selectableSubObject;
    attribute(format=rgba8) float4 color;
}

sampler DebugSampler
{
	MagFilter = Linear,
	MinFilter = Linear,
	MipFilter = None,
	AddressU = Clamp,
	AddressV = Clamp,
	AddressW = Clamp,
	MaxLod = 0,
}

descriptor DebugFragmentPass
{
    ConstantBuffer
    {
        float4x4 WorldToScreen; // camera
        vec3 CameraPosition;
        float _padding0;
		vec2 PixelSizeInScreenUnits;
		vec2 ProjectZ;
    }

    //Texture2D CustomTexture; 

	attribute(sampler=DebugSampler) Texture2DArray TextureAtlas;
    attribute(layout=DebugAtlasEntry) Buffer TextureAtlasEntries;

	attribute(sampler = DebugSampler) Texture2DArray GlyphAtlas;
	attribute(layout = DebugAtlasEntry) Buffer GlyphAtlasEntries;
}

export shader DebugFragmentGS
{
	out vec2 VertexUV;
    out vec4 VertexColor;
	out uint VertexFlags;
	out vec3 WorldPos;
	out vec3 WorldNormal;
	out vec3 TriangleBarycentric;

	attribute(binding=VertexFlags) in uint[] VertexFlagsIn;
	attribute(binding=VertexColor) in vec4[] VertexColorIn;
	attribute(binding=VertexUV) in vec2[] VertexUVIn;
	attribute(binding=VertexSize) in float[] VertexSizeIn;
	attribute(binding=WorldPos) in vec3[] WorldPosIn;
	
	void EmitCopy()
	{
		vec3 ab = WorldPosIn[1] - WorldPosIn[0];
		vec3 ac = WorldPosIn[2] - WorldPosIn[0];
		vec3 n = normalize(cross(ab, ac));

		TriangleBarycentric = vec3(1, 0, 0);
		VertexUV = VertexUVIn[0];
		VertexFlags = VertexFlagsIn[0];
		VertexColor = VertexColorIn[0];
		WorldPos = WorldPosIn[0];
		WorldNormal = n;
		gl_Position = gl_PositionIn[0];
		EmitVertex();
		
		TriangleBarycentric = vec3(0, 1, 0);
		VertexUV = VertexUVIn[1];
		VertexFlags = VertexFlagsIn[1];
		VertexColor = VertexColorIn[1];
		WorldPos = WorldPosIn[1];
		WorldNormal = n;
		gl_Position = gl_PositionIn[1];
		EmitVertex();
		
		TriangleBarycentric = vec3(0, 0, 1);
		VertexUV = VertexUVIn[2];
		VertexFlags = VertexFlagsIn[2];
		VertexColor = VertexColorIn[2];
		WorldPos = WorldPosIn[2];
		WorldNormal = n;
		gl_Position = gl_PositionIn[2];
		EmitVertex();
		
		EndPrimitive();
	}

	void EmitLineVertex(int i, vec3 deltaScreen)
	{
		TriangleBarycentric = vec3(0, 0, 0);
		VertexUV = VertexUVIn[i];
		VertexFlags = 0;
		VertexColor = VertexColorIn[i];
		WorldPos = WorldPosIn[i];
		WorldNormal = vec3(0, 0, 1);


		//--
		// sw = vz
		// sz = vz * m[2][2] + m[2][3]
		//--
		// vz = sw
		
		vec2 screenPos = gl_PositionIn[i].xy / gl_PositionIn[i].w; // calculate final position

		float newW = gl_PositionIn[i].w + deltaScreen.z;
		if (newW > 0.01f)
			newW -= 0.005f;

		float newZ = (newW * ProjectZ.x) + ProjectZ.y;

		vec2 newXY = (screenPos.xy + deltaScreen.xy) * newW;

		gl_Position = vec4(newXY, newZ, newW);

		// TODO ifdef
		gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;

		EmitVertex();
	}

	void EmitLine()
	{
		vec2 width = PixelSizeInScreenUnits.xy * VertexSizeIn[1];

		if (length(gl_PositionIn[2].xy - gl_PositionIn[1].xy) < 0.0001f)
			return;

		vec3 p0 = gl_PositionIn[1].xyz / gl_PositionIn[1].w;
		vec3 p1 = gl_PositionIn[2].xyz / gl_PositionIn[2].w;
		
		vec2 dir = normalize(p1.xy - p0.xy);
		float len = length(p1.xy - p0.xy);

		float dz = (gl_PositionIn[2].w - gl_PositionIn[1].w) / len; // Z change per unit change

		vec3 dx = vec3(-dir.y * width.x, dir.x * width.y, 0.0f); // sideways move does not change the depth
		vec3 dy = vec3(dir.x * width.x, dir.y * width.y, dz * length(width));

		EmitLineVertex(1, dx - dy);
		EmitLineVertex(1, -dx - dy);
		EmitLineVertex(2, dx + dy);
		EmitLineVertex(2, -dx + dy);

		EndPrimitive();
	}

	void EmitSpriteVertex(vec2 deltaScreen, vec2 uv)
	{
		TriangleBarycentric = vec3(0, 0, 0);
		VertexUV = uv;
		VertexFlags = VertexFlagsIn[1];
		VertexColor = VertexColorIn[1];
		WorldPos = WorldPosIn[1];
		WorldNormal = vec3(0,0,1);
		gl_Position = gl_PositionIn[1] + (deltaScreen.xy00) * gl_PositionIn[1].w;

		EmitVertex();
	}

	void EmitSprite()
	{
		vec2 w = PixelSizeInScreenUnits.xy * VertexSizeIn[1];
		vec3 n = vec3(0, 0, 0);

		//vec2 maxScreenSize = PixelSizeInScreenUnits.xy * 256.0f;

		vec2 dx = vec2(w.x, 0);
		vec2 dy = vec2(0, w.y);

		EmitSpriteVertex(-dx + dy, vec2(0,1));
		EmitSpriteVertex(dx + dy, vec2(1, 1));
		EmitSpriteVertex(-dx - dy, vec2(0, 0));
		EmitSpriteVertex(dx - dy, vec2(1, 0));

		EndPrimitive();
	}

	void EmitScreenVertex(int i)
	{
		VertexUV = VertexUVIn[i];
		VertexFlags = VertexFlagsIn[i];
		VertexColor = VertexColorIn[i];
		WorldPos = WorldPosIn[i];
		WorldNormal = vec3(0, 0, 1);

		gl_Position.xy = vec2(-1, -1) + WorldPos.xy * PixelSizeInScreenUnits.xy * 2.0f;
		gl_Position.z = 0.5f;
		gl_Position.w = 1.0f;

		EmitVertex();
	}

	void EmitScreen()
	{
		EmitScreenVertex(0);
		EmitScreenVertex(1);
		EmitScreenVertex(2);
		EmitVertex();
	}
	
	attribute(input=triangles, output=triangle_strip, max_vertices=4)
	void main()
	{
		uint flags = VertexFlagsIn[0];
		if (flags & 16)
			EmitScreen();
		else if (flags & 8)
			EmitLine();
		else if (flags & 4)
			EmitSprite();
		else
			EmitCopy();
	}
}

export shader DebugFragmentVS
{
    vertex DebugVertex v;

    out vec2 VertexUV;
    out vec4 VertexColor;
	out uint VertexFlags;
	out float VertexSize;
	out vec3 WorldPos;
   
    void main()
    {
        WorldPos = v.pos.xyz;
        VertexUV = v.uv;
        VertexColor = v.color;
		VertexFlags = v.flags;
		VertexSize = v.size;
		gl_Position = WorldToScreen * v.pos.xyz1;
    }
}

#define MOD3 vec3(0.1031, 0.11369, 0.13787)

export shader DebugFragmentPS
{
    in vec4 VertexColor;
    in vec2 VertexUV;
	in vec3 WorldPos;
	in vec3 WorldNormal;
	
	attribute(flat) in uint VertexFlags;

	float hash31(vec3 p3)
	{
		p3 = frac(p3 * MOD3);
		p3 += dot(p3, p3.yzx + 19.19);
		return -1.0 + 2.0 * frac((p3.x + p3.y) * p3.z);
	}

	vec3 hash33(vec3 p3)
	{
		p3 = frac(p3 * MOD3);
		p3 += dot(p3, p3.yxz + 19.19);
		return -1.0 + 2.0 * frac(vec3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
	}

	float perlin_noise(vec3 p)
	{
		vec3 pi = floor(p);
		vec3 pf = p - pi;

		vec3 w = pf * pf * (3.0 - 2.0 * pf);

		return 	lerp(
			lerp(
				lerp(dot(pf - vec3(0, 0, 0), hash33(pi + vec3(0, 0, 0))),
					dot(pf - vec3(1, 0, 0), hash33(pi + vec3(1, 0, 0))),
					w.x),
				lerp(dot(pf - vec3(0, 0, 1), hash33(pi + vec3(0, 0, 1))),
					dot(pf - vec3(1, 0, 1), hash33(pi + vec3(1, 0, 1))),
					w.x),
				w.z),
			lerp(
				lerp(dot(pf - vec3(0, 1, 0), hash33(pi + vec3(0, 1, 0))),
					dot(pf - vec3(1, 1, 0), hash33(pi + vec3(1, 1, 0))),
					w.x),
				lerp(dot(pf - vec3(0, 1, 1), hash33(pi + vec3(0, 1, 1))),
					dot(pf - vec3(1, 1, 1), hash33(pi + vec3(1, 1, 1))),
					w.x),
				w.z),
			w.y);
	}

	in vec3 TriangleBarycentric;

	const float LINE_WIDTH = 1.0;

	float CalcEdgeFactor()
	{
		vec3 d = fwidth(TriangleBarycentric);
		vec3 f = step(d * LINE_WIDTH, TriangleBarycentric);
		float fade = saturate(length(d) / 1.0);
		return lerp(min(min(f.x, f.y), f.z), 1, fade);
	}

    void main()
    {
		gl_Target0 = VertexColor;

		if (VertexFlags & 32)
		{
			uint index = VertexFlags >> 16;
			vec2 uvOffset = TextureAtlasEntries[index].uvMin;
			vec2 uvScale = TextureAtlasEntries[index].uvScale;
			vec2 uv = uvOffset + (saturate(VertexUV.xy) * uvScale);

			uint page = TextureAtlasEntries[index].pageIndex;
			gl_Target0 = texture(TextureAtlas, vec3(uv, page));

#ifdef SOLID
			gl_Target0.xyz *= VertexColor.xyz;
			if (gl_Target0.w < 0.55)
				discard;
#elif defined(TRANSPARENT)
			gl_Target0.xyzw *= VertexColor.xyzw;
#endif
		}
		else if (VertexFlags & 64)
		{
			uint index = VertexFlags >> 16;
			vec2 uvOffset = GlyphAtlasEntries[index].uvMin;
			vec2 uvScale = GlyphAtlasEntries[index].uvScale;
			vec2 uv = uvOffset + (saturate(VertexUV.xy) * uvScale);

			uint page = GlyphAtlasEntries[index].pageIndex;
			float alpha = texture(GlyphAtlas, vec3(uv, page)).x;
			gl_Target0.a *= alpha;
		}
		
		if (VertexFlags & 1)
		{
			vec3 N = normalize(WorldNormal);
			vec3 L = normalize(vec3(1.2,1.4,1));
        
			vec3 V = normalize(CameraPosition - WorldPos);
			vec3 H = normalize(V + L);
        
			float NdotL = 0.8 + 0.4*dot(N, L);
			float NdotH = pow(saturate(dot(N,H)), 32) * saturate(dot(N, L));
        
			float diffuseScale = 0.9 + 0.05 * perlin_noise(WorldPos * 10.0);
			float specularScale = 2.5 + 1.0 * perlin_noise(WorldPos * 7.0);
			gl_Target0 = (VertexColor * NdotL * diffuseScale) + (specularScale.xxx0 * NdotH);
		}

		if (VertexFlags & 2)
		{
			gl_Target0 *= CalcEdgeFactor();
		}

#if defined(TRANSPARENT)
		gl_Target0.xyz *= gl_Target0.w; // premultiply alpha
#endif
	}
}

//----

shader DebugFragmentSelectionGatherPS : SelectionGatherPS
{
    attribute(flat) in uint ParamsID;
    attribute(flat) in uint SubSelectableID;
    in float PixelDepth;

    void main()
    {
        //DebugObjectParams params = ObjectParams[ParamsID];
        //emitSelection(params.selectableID, SubSelectableID, PixelDepth);
    }
}

//----
