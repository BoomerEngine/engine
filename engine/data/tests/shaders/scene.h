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

struct CameraInfo
{
	mat4 WorldToScreen;
	vec3 Position;
	float _padding0;
};

descriptor SceneGlobalParams
{
    ConstantBuffer
    {
		CameraInfo[6] Camera;
		vec3 LightDirection;
        vec4 LightColor;
        vec4 AmbientColor;

		uint NumCameras;
		uint _Padding0;
		uint _Padding1;
		uint _Padding2;
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

shader ScenePS
{
	in vec3 WorldPosition;
	in vec3 WorldNormal;
	in vec2 UV;

	void main()
	{
	    float shadow = ShadowSampler.CalcShadow(WorldPosition);

		float NdotL = saturate(dot(WorldNormal, LightDirection));
		vec4 lightColor = AmbientColor + NdotL*LightColor*shadow;

		vec3 V = normalize(Camera[0].Position - WorldPosition);
		vec3 H = normalize(V + LightDirection);
		float HdotN = saturate(dot(WorldNormal, H));
		float spec = pow(HdotN, 40.0) * shadow;

		vec4 albedo = texture(Texture, UV) * DiffuseColor;
		gl_Target0 = lightColor.xyz1 * albedo + SpecularColor * spec;
	}
}

shader SceneVS
{
    vertex Mesh3DVertex v;

	out vec3 WorldPosition;
	out vec3 WorldNormal;
	out vec2 UV;

	void main()
	{
		WorldPosition = (LocalToWorld * v.pos.xyz1).xyz;
		WorldNormal = (LocalToWorld * v.normal.xyz0).xyz;
		UV = v.uv * UVScale;

		gl_Position = Camera[0].WorldToScreen * WorldPosition.xyz1;
	}
}
