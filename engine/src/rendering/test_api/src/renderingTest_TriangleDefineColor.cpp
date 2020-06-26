/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"
#include "renderingTestShared.h"

#include "base/io/include/absolutePath.h"

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {
        /// a basic rendering test
        class RenderingTest_TriangleDefineColor : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TriangleDefineColor, IRenderingTest);

        public:
            RenderingTest_TriangleDefineColor();

            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) override final;

        private:
            BufferView m_vertexBuffer;
            const ShaderLibrary* m_shaderRed;
            const ShaderLibrary* m_shaderGreen;
            const ShaderLibrary* m_shaderBlue;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_TriangleDefineColor);
            RTTI_METADATA(RenderingTestOrderMetadata).order(11);
        RTTI_END_TYPE();

        //---       
        RenderingTest_TriangleDefineColor::RenderingTest_TriangleDefineColor()
        {
        }

        void RenderingTest_TriangleDefineColor::initialize()
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

                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = sizeof(vertices);

                auto sourceData = CreateSourceDataRaw(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
            }

            m_shaderRed = loadShader("TriangleDefineColorRed.csl");
            m_shaderGreen = loadShader("TriangleDefineColorGreen.csl");
            m_shaderBlue = loadShader("TriangleDefineColorBlue.csl");
        }

        void RenderingTest_TriangleDefineColor::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            cmd.opBindVertexBuffer("Vertex2D"_id, m_vertexBuffer);
            cmd.opDraw(m_shaderRed, 0, 3);
            cmd.opDraw(m_shaderGreen, 3, 3);
            cmd.opDraw(m_shaderBlue, 6, 3);
            cmd.opEndPass();
        }

    } // test
} // rendering