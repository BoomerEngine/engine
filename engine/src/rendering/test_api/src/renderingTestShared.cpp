/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "renderingTestShared.h"

namespace rendering
{
    namespace test
    {
        //--

        void DrawQuad(command::CommandWriter& cmd, const ShaderLibrary* func, float x, float y, float w, float h, float u0, float v0, float u1, float v1, base::Color color)
        {
            Simple3DVertex v[6];

            v[0].set(x, y, 0.5f, u0, v0, color);
            v[1].set(x + w, y, 0.5f, u1, v0), color;
            v[2].set(x + w, y + h, 0.5f, u1, v1, color);

            v[3].set(x, y, 0.5f, u0, v0, color);
            v[4].set(x + w, y + h, 0.5f, u1, v1, color);
            v[5].set(x, y + h, 0.5f, u0, v1, color);

            TransientBufferView tempVerticesBuffer(BufferViewFlag::Vertex, TransientBufferAccess::NoShaders, sizeof(v));
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            cmd.opAllocTransientBufferWithData(tempVerticesBuffer, &v, sizeof(v));
            cmd.opBindVertexBuffer("Simple3DVertex"_id, tempVerticesBuffer);
            cmd.opDraw(func, 0, ARRAY_COUNT(v));
        }

        void SetQuadParams(command::CommandWriter& cmd, float x, float y, float w, float h)
        {
            base::Vector4 rectCoords;
            rectCoords.x = -1.0f + (x) * 2.0f;
            rectCoords.y = -1.0f + (y) * 2.0f;
            rectCoords.z = -1.0f + (x+w) * 2.0f;
            rectCoords.w = -1.0f + (y+h) * 2.0f;
            cmd.opBindParametersInline("QuadParams"_id, cmd.opUploadConstants(rectCoords));
        }

        void SetQuadParams(command::CommandWriter& cmd, const ImageView& rt, const base::Rect& rect)
        {
            const auto invWidth = 1.0f / rt.width();
            const auto invHeight = 1.0f / rt.height();

            base::Vector4 rectCoords;
            rectCoords.x = -1.0f + (rect.min.x * invWidth) * 2.0f;
            rectCoords.y = -1.0f + (rect.min.y * invHeight) * 2.0f;
            rectCoords.z = -1.0f + (rect.max.x * invWidth) * 2.0f;
            rectCoords.w = -1.0f + (rect.max.y * invHeight) * 2.0f;
            cmd.opBindParametersInline("QuadParams"_id, cmd.opUploadConstants(rectCoords));
        }                

        //--

    } // test
} // rendering