/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#include "build.h"
#include "gameEffects.h"

namespace example
{
    //---

    static base::res::StaticResource<ShaderLibrary> resSkyShader("/examples/canvas/shaders/plasma_sky.fx");

    struct SkyParams
    {
        struct Constants
        {
            int targetSizeX = 1;
            int targetSizeY = 1;
            float centerX = 0.0f;
            float centerY = 0.0f;
            float time = 0.0f;
        };

        ConstantsView consts;
    };


    void RenderSky(CommandWriter& cmd, uint32_t width, uint32_t height, Vector2 center, float time)
    {
        SkyParams::Constants data;
        data.targetSizeX = width;
        data.targetSizeY = height;
        data.centerX = center.x;
        data.centerY = center.y;
        data.time = time;

        command::CommandWriterBlock block(cmd, "Sky");

        if (auto shader = resSkyShader.loadAndGet())
        {
            SkyParams params;
            params.consts = cmd.opUploadConstants(data);
            cmd.opBindParametersInline("SkyParams"_id, params);

            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
            cmd.opDraw(shader, 0, 4);
        }
    }

    //--

} // example
