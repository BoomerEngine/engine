/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

#pragma once

//--

attribute(packing=vertex) struct Mesh3DVertex
{
    attribute(offset=0) vec3 pos;
    attribute(offset=12) vec3 normal;
    attribute(offset=24) vec2 uv;
    attribute(offset=32, format=rgba8) vec4 color;
}

//--

descriptor SceneObjectParams
{
    ConstantBuffer
    {
        attribute(offset=0) mat4 LocalToWorld;
        attribute(offset=64) vec2 UVScale;
        attribute(offset=80) vec4 DiffuseColor;
        attribute(offset=96) vec4 SpecularColor;
    }

	attribute(sampler=SamplerWrapLinear) Texture2D Texture;
}

//--

descriptor SceneGlobalParams
{
    ConstantBuffer
    {
        attribute(offset=0) mat4 WorldToScreen;
        attribute(offset=64) vec3 CameraPosition;
        attribute(offset=80) vec3 LightDirection;
        attribute(offset=96) vec4 LightColor;
        attribute(offset=112) vec4 AmbientColor;
    }
}

//--

shader ShadowSampler
{
    float CalcShadow(vec3 pos)
    {
        return 1.0f;
    }
}

//--

export shader ScenePS
{
	in vec3 WorldPosition;
	in vec3 WorldNormal;
	in vec2 UV;

	void main()
	{
	    float shadow = ShadowSampler.CalcShadow(WorldPosition);

		float NdotL = saturate(dot(WorldNormal, LightDirection));
		vec4 lightColor = AmbientColor + NdotL*LightColor*shadow;

		vec3 V = normalize(CameraPosition - WorldPosition);
		vec3 H = normalize(V + LightDirection);
		float HdotN = saturate(dot(WorldNormal, H));
		float spec = pow(HdotN, 40.0) * shadow;

		vec4 albedo = texture(Texture, UV) * DiffuseColor;
		gl_Target0 = lightColor.xyz1 * albedo + SpecularColor * spec;
	}
}

export shader SceneVS
{
    vertex Mesh3DVertex v;

	out vec3 WorldPosition;
	out vec3 WorldNormal;
	out vec2 UV;

	void main()
	{
		WorldPosition = (LocalToWorld * v.pos.xyz1).xyz;
		gl_Position = WorldToScreen * WorldPosition.xyz1;

		WorldNormal = (LocalToWorld * v.normal.xyz0).xyz;
		UV = v.uv * UVScale;
	}
}
