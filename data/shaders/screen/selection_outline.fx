/***
* Inferno Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Selection outline 
*
***/

//----

#include <math.h>
#include <frame.h>
#include <camera.h>

//----

sampler OutlineDepthSampler
{
    MinFilter = Nearest,
    MagFilter = Nearest,
    MipFilter = None,
    AddressU = Clamp,
    AddressV = Clamp,
    AddressW = Clamp,
}

state OutlineRenderStates
{
    PrimitiveTopology = TriangleStrip,

    BlendingEnabled = true,
    BlendColorSrc0 = SrcAlpha,
    BlendAlphaSrc0 = SrcAlpha,
    BlendColorDest0 = OneMinusSrcAlpha,
    BlendAlphaDest0 = OneMinusSrcAlpha,
}

descriptor SelectionOutlineParams
{
    ConstantBuffer
    {
        vec2 SourceUVOffset;
        vec2 SourceUVScale;

        vec4 ColorFront;
        vec4 ColorBack;
        int OutlineSize;
        float CenterOpacity;
    }

    attribute(sampler=OutlineDepthSampler) Texture2D SceneDepthBufferTexture;
    attribute(sampler=OutlineDepthSampler) Texture2D SelectionDepthBufferTexture;
};

attribute(state=OutlineRenderStates)
export shader PS
{
    float CalcSelectionDepthNearby(ivec2 pixelPos)
    {
        float selectionDepth = 1.0;

        int size = 4;// OutlineSize;
        for (int dy=-size; dy<=size; dy += 1)
        {
            for (int dx=-size; dx<=size; dx += 1)
            {
                float selectionScreenDepth = textureLoadLodOffset(SelectionDepthBufferTexture, pixelPos.xy, 0, vec2(dx,dy)).x;
                if (selectionScreenDepth > 0.0 && selectionScreenDepth < 1.0)
                    selectionDepth = min(selectionDepth, selectionScreenDepth);
            }
        }

        return selectionDepth;
    }

    vec4 CalcSelectionColor(vec2 sourcePixelPos, ivec2 targetPixelPos)
    {
        ivec2 pixelPos = ivec2(sourcePixelPos);

        float sceneScreenDepth = textureLoadLod(SceneDepthBufferTexture, pixelPos, 0).x;
        float sceneLinearZ = Camera.CalcLinearizeViewDepth(sceneScreenDepth);

        float centerSelectionDepth = textureLoadLod(SelectionDepthBufferTexture, pixelPos.xy, 0).x;
        float centerSelectionLinearZ = Camera.CalcLinearizeViewDepth(centerSelectionDepth);

        //return vec4(pixelPos.xy * 1.0, 0, 1);
        //return vec4(frac(centerSelectionDepth * 10000.0) + 0.3);
        
        if (centerSelectionDepth == 1.0)
        {
            float selectionDepth = CalcSelectionDepthNearby(pixelPos);
            if (selectionDepth < 1.0)
            {
                bool grid = (targetPixelPos.x & 1) ^ (targetPixelPos.y & 1);
                if ((selectionDepth <= sceneScreenDepth))
                    return ColorFront;
                else if (grid)
                    return ColorBack;
            }
        }
        else if (centerSelectionDepth > 0.0f && centerSelectionDepth < 1.0f)
        {            
            if (abs(sceneLinearZ - centerSelectionLinearZ) < 0.005)
            {
                return vec4(ColorFront.xyz * CenterOpacity, CenterOpacity * 0.25f);
            }
        }

        return vec4(0);
    }

    void main()
    {
        ivec2 targetPixelCoord = gl_FragCoord.xy;
        vec2 sourcePixelCoors = (targetPixelCoord * SourceUVScale) + SourceUVOffset;
        gl_Target0 = CalcSelectionColor(sourcePixelCoors, targetPixelCoord);
        //gl_Target0.w = 1;
    }
}

export shader VS
{
    void main()
    {
        gl_Position.x = (gl_VertexID & 1) ? -1.0f : 1.0f;
        gl_Position.y = (gl_VertexID & 2) ? -1.0f : 1.0f;
        gl_Position.z = 0.5f;
        gl_Position.w = 1.0f;
    }
}
