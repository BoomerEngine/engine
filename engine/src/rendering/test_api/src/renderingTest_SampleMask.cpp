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
#include "rendering/device/include/renderingImage.h"
#include "rendering/device/include/renderingDeviceGlobalObjects.h"

namespace rendering
{
    namespace test
    {
        /// sample mask test
        class RenderingTest_SampleMask : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_SampleMask, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

        private:
			GraphicsPassLayoutObjectPtr m_renderToTextureLayout;

            GraphicsPipelineObjectPtr m_shaderDraw;
			GraphicsPipelineObjectPtr m_shaderGenerate;

            uint8_t m_sampleCount = 1;

			ImageObjectPtr m_colorBuffer;
			RenderTargetViewPtr m_colorBufferRTV;

			ImageObjectPtr m_resolvedColorBuffer;
			ImageSampledViewPtr m_resolvedColorBufferSRV;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_SampleMask);
            RTTI_METADATA(RenderingTestOrderMetadata).order(1500);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(5);
        RTTI_END_TYPE();

        //---       

        void RenderingTest_SampleMask::initialize()
        {
            m_sampleCount = 1 << subTestIndex();

			{
				GraphicsPassLayoutSetup setup;
				setup.samples = m_sampleCount;
				setup.color[0].format = ImageFormat::RGBA8_UNORM;
				m_renderToTextureLayout = createPassLayout(setup);
			}

            m_shaderDraw = loadGraphicsShader("SampleMaskPreview.csl", outputLayoutNoDepth());
			m_shaderGenerate = loadGraphicsShader(base::TempString("SampleMaskGenerate{}.csl", subTestIndex()), m_renderToTextureLayout);
        }

        void RenderingTest_SampleMask::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
            if (m_colorBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.format = rendering::ImageFormat::RGBA8_UNORM;
                info.width = backBufferView->width();
                info.height = backBufferView->height();
                info.numSamples = m_sampleCount;

                m_colorBuffer = createImage(info);
				m_colorBufferRTV = m_colorBuffer->createRenderTargetView();
            }

            if (m_resolvedColorBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::RGBA8_UNORM;
                info.width = backBufferView->width();
                info.height = backBufferView->height();

                m_resolvedColorBuffer = createImage(info);
				m_resolvedColorBufferSRV = m_resolvedColorBuffer->createSampledView();
            }

            // draw triangles
            {
                FrameBuffer fb;
                fb.color[0].view(m_colorBufferRTV).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

                cmd.opBeingPass(m_renderToTextureLayout, fb);

                drawQuad(cmd, m_shaderGenerate, -0.9f, -0.9f, 1.8f, 1.8f);

                cmd.opEndPass();
            }

            //--

			cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::RenderTarget, ResourceLayout::ResolveSource);
			cmd.opTransitionLayout(m_resolvedColorBuffer, ResourceLayout::ShaderResource, ResourceLayout::ResolveDest);
            cmd.opResolve(m_colorBuffer, m_resolvedColorBuffer);
			cmd.opTransitionLayout(m_resolvedColorBuffer, ResourceLayout::ResolveDest, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::ResolveSource, ResourceLayout::RenderTarget);

            //--

            {
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
                cmd.opBeingPass(outputLayoutNoDepth(), fb);

				DescriptorEntry entry[1];
				entry[0] = m_resolvedColorBufferSRV;
                cmd.opBindDescriptor("TestParams"_id, entry);

                cmd.opDraw(m_shaderDraw, 0, 4);
                cmd.opEndPass();
            }
        }

    } // test
} // rendering
