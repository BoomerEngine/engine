/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* PBR helpers
*
***/

#pragma once

#include <math.h>

//----

// global light data
struct GlobalLightingParams
{
    vec3 LightDirection; // normal vector towards the global light
    vec3 LightColor; // color of the global light, LINEAR
    vec3 AmbientColorZenith; // color of the global ambient light, LINEAR
    vec3 AmbientColorHorizon; // color of the global light, LINEAR
};

// lighting data
descriptor LightingParams
{
    ConstantBuffer 
    {
        GlobalLightingParams GlobalLighting;
    }
};

//----

//----