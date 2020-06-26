/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
***/

#pragma once

//----

const float PI = 3.1415926535897932f;
const float TWOPI = 2.0f * PI;

//----

// a simple 2D vertex to be used with tests
attribute(packing=vertex) struct Vertex2D
{
    attribute(offset=0) vec2 pos;
}


// a simple 3D vertex to be used with tests
attribute(packing=vertex) struct Simple3DVertex
{
    attribute(offset=0)                 vec3 pos;
    attribute(offset=12)                vec2 uv;
    attribute(offset=20, format=rgba8)  vec4 color;
}

//----

// https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/ne-d3d11-d3d11_standard_multisample_quality_levels
const vec2[2] MSAASampleLocations2 = array(
    vec2(4.0/8.0, 4.0/8.0),
    vec2(-4.0/8.0, -4.0/8.0));

const vec2[4] MSAASampleLocations4 = array(
    vec2(-2.0/8.0, -6.0/8.0),
    vec2(6.0/8.0, -2.0/8.0),
    vec2(-6.0/8.0, 2.0/8.0),
    vec2(2.0/8.0, 6.0/8.0));

const vec2[8] MSAASampleLocations8 = array(
    vec2(1.0/8.0, -3.0/8.0),
    vec2(-1.0/8.0, 3.0/8.0),
    vec2(5.0/8.0, 1.0/8.0),
    vec2(-3.0/8.0, 5.0/8.0),
    vec2(-5.0/8.0, 5.0/8.0),
    vec2(-7.0/8.0, -1.0/8.0),
    vec2(3.0/8.0, 7.0/8.0),
    vec2(7.0/8.0, -7.0/8.0));

vec2 CalcSampleOffset(uint NumSamples, int i)
{
    if (NumSamples == 2)
        return MSAASampleLocations2[i];
    else if (NumSamples == 4)
        return MSAASampleLocations4[i];
    else if (NumSamples == 8)
        return MSAASampleLocations8[i];

    return vec2(0,0);
}

//----

float Luminance(vec3 linearColor)
{
    return dot(linearColor, vec3(0.3f, 0.59f, 0.11f));
}

//----

descriptor QuadParams
{
    ConstantBuffer
    {
		vec4 Rect;
    }
}

//--

shader QuadVS
{
	void main()
	{
		gl_Position.x = (gl_VertexID & 1) ? Rect.x : Rect.z;
		gl_Position.y = (gl_VertexID & 2) ? Rect.y : Rect.w;	
		gl_Position.z = 0.5f;
		gl_Position.w = 1.0f;
	}
}

//----