/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Debug Geometry Rendering Pipeline
*
***/

#include <math.h>
#include <selection.h>

attribute(packing=vertex) struct DebugVertex
{
    float3 pos;
    float2 uv;
    attribute(format=rgba8) float4 color;
}

descriptor DebugFragmentPass
{
    ConstantBuffer
    {
        float4x4 WorldToScreen; // camera
        vec4 _padding0;
    }

    //Texture2D CustomTexture; 
    //Texture2DArray TextureAtlas;
    //attribute(layout=DebugObjectParams, uav) Buffer ObjectParams;
}

export shader DebugFragmentVS
{
    vertex DebugVertex v;

    out vec2 UV;
    out vec4 Color;
   
    void main()
    {
        gl_Position = WorldToScreen * v.pos.xyz1;

        UV = v.uv;
        Color = v.color;

        // TODO: z-bias
    }
}

export shader DebugFragmentPS
{
    in vec4 Color;
    in vec2 UV;

    attribute(flat) in uint ParamsID;

    void main()
    {
        gl_Target0 = Color;
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
