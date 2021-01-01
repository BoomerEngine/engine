/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Common vertex support for materials
*
***/

#pragma once

//----

struct ObjectInfo
{
    mat4 LocalToScene; // transformation matrix
    vec4 SceneBoundsCenter; // .w = free
    uint SelectionObjectID;
    uint SelectionSubObjectID;
    uint Color;
    uint ColorEx;
};

//----

vec3 UnpackPosition_11_11_10(uint data)
{
    float x = (data & 2047) / 2047.0f;
    float y = ((data >> 11) & 2047) / 2047.0f;
    float z = ((data >> 22) & 1023) / 1023.0f;

    return vec3(x, y, z);
}

vec3 UnpackPosition_11_11_10_NotNorm(uint data)
{
    float x = (data & 2047);
    float y = ((data >> 11) & 2047);
    float z = ((data >> 22) & 1023);

    return vec3(x, y, z);
}

vec3 UnpackPosition_22_22_20(uint dataLo, uint dataHi)
{
    float x = (dataLo & 4194303);
    float y = ((dataLo >> 22) | ((dataHi & 4095) << 10)) & 4194303;
    float z = (dataHi >> 12) & 1048575;

    return vec3(x, y, z);
}

vec3 UnpackNormalVector(uint data)
{
    return normalize((UnpackPosition_11_11_10(data) * 2.0) - 1.0);
}

vec4 UnpackColorRGBA4(uint data)
{
    return vec4(1);//unpackUnorm4x8(data);
}

vec2 UnpackHalf2(uint data)
{
    return unpackHalf2x16(data);
}

//----

export shader MaterialVS
{

}

//----