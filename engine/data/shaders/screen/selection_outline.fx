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

descriptor SelectionOutlineParams
{
    ConstantBuffer
    {
        vec4 ColorFront;
        vec4 ColorBack;
        int OutlineSize;
        float CenterOpacity;
    }

    Texture2D SceneDepthBufferTexture;
    Texture2D SelectionDepthBufferTexture;    
};

export shader PS
{
    float CalcSelectionDepthNearby(ivec2 pixelPos)
    {
        float selectionDepth = 1.0;

        int size = OutlineSize;
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

    vec4 CalcSelectionColor(ivec2 pixelPos)
    {
        float sceneScreenDepth = textureLoadLod(SceneDepthBufferTexture, pixelPos, 0).x;
        float sceneLinearZ = Camera.CalcLinearizeViewDepth(sceneScreenDepth);

        float centerSelectionDepth = textureLoadLod(SelectionDepthBufferTexture, pixelPos.xy, 0).x;
        float centerSelectionLinearZ = Camera.CalcLinearizeViewDepth(centerSelectionDepth);
        
        if (centerSelectionDepth == 1.0)
        {
            float selectionDepth = CalcSelectionDepthNearby(pixelPos);
            if (selectionDepth < 1.0)
            {
                bool grid = (pixelPos.x & 1) ^ (pixelPos.y & 1);
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
        ivec2 pixelCoord = gl_FragCoord.xy;
        gl_Target0 = CalcSelectionColor(pixelCoord);
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
