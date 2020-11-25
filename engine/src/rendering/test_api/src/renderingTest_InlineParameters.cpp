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
        /// a basic test of inline parameters
        class RenderingTest_InlineParameters : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_InlineParameters, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

        private:
            BufferObjectPtr m_vertexBuffer;
            ShaderLibraryPtr m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_InlineParameters);
        RTTI_METADATA(RenderingTestOrderMetadata).order(70);
        RTTI_END_TYPE();

        //---       

        void RenderingTest_InlineParameters::initialize()
        {
            m_shaders = loadShader("InlineParameters.csl");

            // create vertex buffer with a single triangle
            {
                Simple3DVertex vertices[9];
                vertices[0].set(0, -1, 0.5f, 0, 0, base::Color::WHITE);
                vertices[1].set(-1, 1, 0.5f, 0, 0, base::Color::WHITE);
                vertices[2].set(1, 1, 0.5f, 0, 0, base::Color::WHITE);
                m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
            }
        }

        void RenderingTest_InlineParameters::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
			//--

			struct InlineParameterConsts
			{
				base::Vector2 TestOffset = base::Vector2(0, 0);
				base::Vector2 TestScale = base::Vector2(1, 1);
			};

			struct InlineParameterConstsEx
			{
				base::Vector4 TestColor = base::Vector4(1, 1, 1, 1);
			};

			//--

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);

            // draw array of triangle
            uint32_t numTrianglesWidth = 64;
            uint32_t numTrianglesHeight = 64;
            for (uint32_t y = 0; y < numTrianglesHeight; ++y)
            {
                float fracY = y / (float) (numTrianglesHeight - 1);
                for (uint32_t x = 0; x < numTrianglesWidth; ++x)
                {
                    float fracX = x / (float) (numTrianglesWidth - 1);

                    InlineParameterConsts params;
                    params.TestScale.x = 1.0f / (float) numTrianglesWidth;
                    params.TestScale.y = 1.0f / (float) numTrianglesHeight;
                    params.TestOffset.x = -1.0f + 2.0f * params.TestScale.x * (0.5f + x);
                    params.TestOffset.y = -1.0f + 2.0f * params.TestScale.y * (0.5f + y);

                    InlineParameterConstsEx paramsEx;
                    paramsEx.TestColor.x = fracX;
                    paramsEx.TestColor.y = fracY;
                    paramsEx.TestColor.z = 0.0f;
                    paramsEx.TestColor.w = 1.0f;

					DescriptorEntry desc[2];
					desc[0].constants(params);
					desc[1].constants(paramsEx);
                    cmd.opBindDescriptor("TestParams"_id, desc);

                    cmd.opDraw(m_shaders, 0, 3);
                }
            }

            cmd.opEndPass();
        }

    } // test
} // rendering