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
        vec3 CameraPosition;
        float _padding0;
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
	out vec3 WorldPos;
   
    void main()
    {
        gl_Position = WorldToScreen * v.pos.xyz1;
        
        WorldPos = v.pos.xyz;
        UV = v.uv;
        Color = v.color;

        // TODO: z-bias
    }
}

export shader DebugFragmentPS
{
    in vec4 Color;
	in vec3 WorldPos;
    in vec2 UV;

    void main()
    {
        vec3 N = -normalize(cross(ddx(WorldPos), ddy(WorldPos)));
        vec3 L = normalize(vec3(1,1,1));
        
        vec3 V = normalize(CameraPosition - WorldPos);
        vec3 H = normalize(V + L);
        
        float NdotL = 0.8 + 0.2*dot(N, L);
        float NdotH = pow(saturate(dot(N,H)), 32);
        
        gl_Target0 = (Color * NdotL) + (vec4(2,2,2,0) * NdotH);
        //gl_Target0 = (0.5f + 0.5f*N).xyz1;
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
