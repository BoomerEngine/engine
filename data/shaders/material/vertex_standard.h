/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Standalone VB/IB mesh support
*
***/

#pragma once

//----

#include "vertex.h"

descriptor InstanceDataDesc
{
    ConstantBuffer
    {
        vec4 QuantizationOffset;
        vec4 QuantizationScale;
        
        ObjectInfo[1] ObjectData;
    }
}

//----

export shader MaterialVS
{
    uint FetchObjectIndex()
    {
        return gl_InstanceID;
    }

    vec3 DecompressQuantizedPosition(vec3 pos)
    {
        return (QuantizationScale.xyz * pos) + QuantizationOffset.xyz;
    }
}

//----