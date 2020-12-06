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
        class RenderingTest_TriangleDefineColor : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TriangleDefineColor, IRenderingTest);

        public:
            RenderingTest_TriangleDefineColor();

            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

        private:
            BufferObjectPtr m_vertexBuffer;
            GraphicsPipelineObjectPtr m_shaderRed;
			GraphicsPipelineObjectPtr m_shaderGreen;
			GraphicsPipelineObjectPtr m_shaderBlue;
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

                m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
            }

            m_shaderRed = loadGraphicsShader("TriangleDefineColorRed.csl", outputLayoutNoDepth());
            m_shaderGreen = loadGraphicsShader("TriangleDefineColorGreen.csl", outputLayoutNoDepth());
            m_shaderBlue = loadGraphicsShader("TriangleDefineColorBlue.csl", outputLayoutNoDepth());
        }

        void RenderingTest_TriangleDefineColor::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(outputLayoutNoDepth(), fb);
            cmd.opBindVertexBuffer("Vertex2D"_id, m_vertexBuffer);
            cmd.opDraw(m_shaderRed, 0, 3);
            cmd.opDraw(m_shaderGreen, 3, 3);
            cmd.opDraw(m_shaderBlue, 6, 3);
            cmd.opEndPass();
        }

    } // test
} // rendering