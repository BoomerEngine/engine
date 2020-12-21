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
#include "rendering/device/include/renderingBuffer.h"

namespace rendering
{
    namespace test
    {
        /// test of the non-atomic writes to the storage buffer
        class RenderingTest_FormatBufferWrite : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_FormatBufferWrite, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

        private:
            static const uint32_t MAX_ELEMENTS = 1024;

            uint32_t m_vertexCount;
            GraphicsPipelineObjectPtr m_shaderGenerate;
			GraphicsPipelineObjectPtr m_shaderTest;

            BufferObjectPtr m_tempBuffer;
			BufferViewPtr m_tempBufferSRV;
			BufferWritableViewPtr m_tempBufferUAV;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_FormatBufferWrite);
        RTTI_METADATA(RenderingTestOrderMetadata).order(330);
        RTTI_END_TYPE();

        //---       

        void RenderingTest_FormatBufferWrite::initialize()
        {
            m_shaderGenerate = loadGraphicsShader("FormatBufferWriteGenerate.csl");
            m_shaderTest = loadGraphicsShader("FormatBufferWriteTest.csl");

            m_tempBuffer = createStorageBuffer(2 * sizeof(base::Vector4) * MAX_ELEMENTS);
			m_tempBufferUAV = m_tempBuffer->createWritableView(ImageFormat::RGBA32F);
			m_tempBufferSRV = m_tempBuffer->createView(ImageFormat::RGBA32F);
        }

        void RenderingTest_FormatBufferWrite::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            float yScale = 0.05f;
            for (float y = -1.0f; y < 1.0f; y += yScale)
            {
				struct
				{
					float TimeOffset;
					float DrawOffset;
					float DrawScale;
				} tempConsts;

				tempConsts.TimeOffset = time + y * TWOPI;
				tempConsts.DrawOffset = y;
				tempConsts.DrawScale = yScale;

                {
					DescriptorEntry params[2];
					params[0].constants(tempConsts);
					params[1] = m_tempBufferUAV;
					cmd.opBindDescriptor("TestParams"_id, params);

                    cmd.opDraw(m_shaderGenerate, 0, MAX_ELEMENTS);
                }

                cmd.opTransitionLayout(m_tempBuffer, ResourceLayout::UAV, ResourceLayout::ShaderResource);

                {
					DescriptorEntry params[2];
					params[0].constants(tempConsts);
					params[1] = m_tempBufferSRV;
					cmd.opBindDescriptor("TestParams"_id, params);

                    cmd.opDraw(m_shaderTest, 0, MAX_ELEMENTS);
                }

				cmd.opTransitionLayout(m_tempBuffer, ResourceLayout::ShaderResource, ResourceLayout::UAV);
            }

            cmd.opEndPass();
        }

    } // test
} // rendering;