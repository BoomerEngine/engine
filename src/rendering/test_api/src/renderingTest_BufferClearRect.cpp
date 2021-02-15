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
        /// test of the texture buffer read in the shader
        class RenderingTest_BufferClearRect : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_BufferClearRect, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferViewDepth) override final;

        private:
            GraphicsPipelineObjectPtr m_shaders;
			BufferObjectPtr m_extraBuffer;

			BufferViewPtr m_extraBufferSRV;
			BufferWritableViewPtr m_extraBufferUAV;

            uint32_t m_sideCount;

			base::FastRandState m_rand;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_BufferClearRect);
        RTTI_METADATA(RenderingTestOrderMetadata).order(335);
        RTTI_END_TYPE();

        //---       

		void RenderingTest_BufferClearRect::initialize()
        {
            m_sideCount = 64;

            // generate colors
            {
                rendering::BufferCreationInfo info;
                info.allowShaderReads = true;
				info.allowUAV = true;
				info.initialLayout = ResourceLayout::ShaderResource;
                info.size = m_sideCount * m_sideCount * 4;

                m_extraBuffer = createBuffer(info);
				m_extraBufferSRV = m_extraBuffer->createView(ImageFormat::RGBA8_UNORM);
				m_extraBufferUAV = m_extraBuffer->createWritableView(ImageFormat::RGBA8_UNORM);
            }

            m_shaders = loadGraphicsShader("FormatBufferPackedRead.csl");
        }

        void RenderingTest_BufferClearRect::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferViewDepth)
        {
			// clear part
			{
				const auto length = (uint32_t)m_sideCount * m_sideCount;
				auto w = (uint32_t)m_rand.range(1, 100);
				auto x = m_rand.range(length - w);

				base::Color color;
				color.r = m_rand.next();
				color.g = m_rand.next();
				color.b = m_rand.next();
				color.a = 255;

				ResourceClearRect rect[1];
				rect[0].buffer.offset = x * 4;
				rect[0].buffer.size = w * 4;

				cmd.opTransitionLayout(m_extraBuffer, ResourceLayout::ShaderResource, ResourceLayout::UAV);
				cmd.opClearWritableBufferRects(m_extraBufferUAV, &color, rect, 1);
				cmd.opTransitionLayout(m_extraBuffer, ResourceLayout::UAV, ResourceLayout::ShaderResource);
			}

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);		

			struct
			{
				uint32_t sideCount;
			} consts;

			consts.sideCount = m_sideCount;

			DescriptorEntry desc[2];
			desc[0].constants(consts);
			desc[1] = m_extraBufferSRV;
            cmd.opBindDescriptor("TestParams"_id, desc);

			drawQuad(cmd, m_shaders);

            cmd.opEndPass();
        }

    } // test
} // rendering