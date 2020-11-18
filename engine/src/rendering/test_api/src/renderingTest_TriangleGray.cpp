/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {
        /// a basic rendering test
        class RenderingTest_TriangleGray : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TriangleGray, IRenderingTest);

        public:
            RenderingTest_TriangleGray();

            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) override final;

        private:
            BufferView m_vertexBuffer;
            const ShaderLibrary* m_shader;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_TriangleGray);
            RTTI_METADATA(RenderingTestOrderMetadata).order(9);
        RTTI_END_TYPE();

        //---       
        RenderingTest_TriangleGray::RenderingTest_TriangleGray()
        {
        }

        void RenderingTest_TriangleGray::initialize()
        {
            {
                base::Vector2 vertices[9];
                vertices[0] = base::Vector2(0, -0.5f);
                vertices[1] = base::Vector2(-0.7f, 0.5f);
                vertices[2] = base::Vector2(0.7f, 0.5f);

                vertices[3] = base::Vector2(0 - 0.2f, -0.5f);
                vertices[4] = base::Vector2(-0.7f - 0.2f, 0.5f);
                vertices[5] = base::Vector2(0.7f - 0.2f, 0.5f);

                vertices[6] = base::Vector2(0 + 0.2f, -0.5f);
                vertices[7] = base::Vector2(-0.7f + 0.2f, 0.5f);
                vertices[8] = base::Vector2(0.7f + 0.2f, 0.5f);

                m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
            }

            m_shader = loadShader("TriangleGray.csl");
        }

        void RenderingTest_TriangleGray::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            cmd.opBindVertexBuffer("Vertex2D"_id, m_vertexBuffer);
            cmd.opDraw(m_shader, 0, 9);
            cmd.opEndPass();
        }

    } // test
} // rendering