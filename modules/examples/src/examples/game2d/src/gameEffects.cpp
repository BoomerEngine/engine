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

    static base::res::StaticResource<ShaderFile> resSkyShader("/examples/canvas/shaders/plasma_sky.fx");

    void RenderSky(CommandWriter& cmd, GraphicsPassLayoutObject* pass, uint32_t width, uint32_t height, Vector2 center, float time)
    {
		struct
		{
			int targetSizeX = 1;
			int targetSizeY = 1;
			float centerX = 0.0f;
			float centerY = 0.0f;
			float time = 0.0f;
		} data;

        data.targetSizeX = width;
        data.targetSizeY = height;
        data.centerX = center.x;
        data.centerY = center.y;
        data.time = time;

        command::CommandWriterBlock block(cmd, "Sky");

        if (auto shader = resSkyShader.loadAndGet())
        {
			if (auto root = shader->rootShader())
			{
				DescriptorEntry desc[1];
				desc[0].constants(data);
				cmd.opBindDescriptor("SkyParams"_id, desc);

				//cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
				//cmd.opDraw(shader, 0, 4);
			}
        }
    }

    //--

} // example
