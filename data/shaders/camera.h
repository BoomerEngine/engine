/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Camera constants
*
***/

#pragma once

//--

#include <math.h>

//--

// fundamental camera rendering stuff
descriptor CameraParams
{
    ConstantBuffer
    {
        attribute(offset=0) vec3 CameraPosition; // explicit offsets since we are putting vec3 in space for 4 floats
        attribute(offset=16) vec3 CameraForward;
        attribute(offset=32) vec3 CameraUp;
        attribute(offset=48) vec3 CameraRight;

        attribute(offset=64) mat4 WorldToScreen;
        mat4 ScreenToWorld;
        mat4 WorldToScreenNoJitter;
        mat4 ScreenToWorldNoJitter;
        mat4 WorldToPixelCoord;
        mat4 PixelCoordToWorld;

        vec4 LinearizeZ;
        vec4 NearFarPlane;

        vec4 PrevCameraPosition;
        mat4 PrevWorldToScreen;
        mat4 PrevScreenToWorld;
        mat4 PrevWorldToScreenNoJitter;
        mat4 PrevScreenToWorldNoJitter;
    }
}

//--

// all common camera related functions
shader Camera
{
    //---

    vec3 CalcWorldPosition(vec3 screenPos)
    {
        vec4 pos = ScreenToWorld * screenPos.xyz1;
        return pos.xyz / pos.w;
    }

    vec3 CalcWorldPositionNoJitter(vec3 screenPos)
    {
        vec4 pos = mul(ScreenToWorldNoJitter, screenPos.xyz1);
        return pos.xyz / pos.w;
    }

    /*vec3 CalcScreenPositionProj(vec3 worldPos)
    {
        vec4 pos = mul(WorldToScreen, worldPos.xyz1);
        return pos.xyz / pos.w;
    }

    vec4 calcScreenPositionUnproj(vec3 worldPos)
    {
        return mul(WorldToScreen, worldPos.xyz1);
    }

    vec3 calcScreenPositionProjNoJitter(vec3 worldPos)
    {
        vec4 pos = mul(WorldToScreenNoJitter, worldPos.xyz1);
        return pos.xyz / pos.w;
    }

    //---

    vec3 calcPrevScreenPositionProj(vec3 worldPos)
    {
        vec4 pos = mul(PrevWorldToScreen, worldPos.xyz1);
        return pos.xyz / pos.w;
    }

    vec3 calcPrevScreenPositionProjNoJitter(vec3 worldPos)
    {
        vec4 pos = mul(PrevWorldToScreenNoJitter, worldPos.xyz1);
        return pos.xyz / pos.w;
    }*/

    //--

    float CalcLinearizeViewDepth(float screenDepthValue)
    {
        // screenZ = (worldZ * m22 + m23) / worldZ;
        // screenZ = m22 + m23 / worldZ;
        // screenZ - m22 = m23 / worldZ;
        // (screenZ - m22) / m23 = 1 / worldZ;
        // worldZ = m23 / (screenZ - m22);
    
        return LinearizeZ.y / (screenDepthValue - LinearizeZ.x);
    }
}

//--