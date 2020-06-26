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

attribute(packing=vertex) struct DebugVertexEx
{
    uint subSelectionID;
    uint paramsID;
    int depthBias;
    float size; // sprites and lines
}

descriptor DebugFragmentPass
{
    ConstantBuffer
    {
        float4x4 WorldToScreen; // camera
    }

    //Texture2D CustomTexture; 
    //Texture2DArray TextureAtlas;
    //attribute(layout=DebugObjectParams, uav) Buffer ObjectParams;
}

export shader DebugFragmentVS
{
    vertex DebugVertex v;
    vertex DebugVertexEx o;

    out vec2 UV;
    out vec4 Color;
    out float Size;
    out float PixelDepth;
    out uint ParamsID;
    out uint SubSelectableID;
   
    void main()
    {
        gl_Position = WorldToScreen * v.pos.xyz1;

        UV = v.uv;
        Color = v.color;

        ParamsID = o.paramsID;
        SubSelectableID = o.subSelectionID;
        PixelDepth = gl_Position.w;
        Size = o.size;

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
