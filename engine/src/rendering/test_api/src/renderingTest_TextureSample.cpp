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
        /// test of a sampled image
        class RenderingTest_TextureSample : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TextureSample, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

        private:
            ImageSampledViewPtr m_sampledImage;
            GraphicsPipelineObjectPtr m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_TextureSample);
            RTTI_METADATA(RenderingTestOrderMetadata).order(710);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(4);
        RTTI_END_TYPE();

        //---

        void RenderingTest_TextureSample::initialize()
        {
            auto image = loadImage2D("lena.png", true);
			m_sampledImage = image->createSampledView();

            switch (subTestIndex())
            {
                case 0: m_shaders = loadGraphicsShader("TextureSample.csl", outputLayoutNoDepth()); break;
                case 1: m_shaders = loadGraphicsShader("TextureSampleOffset.csl", outputLayoutNoDepth()); break;
                case 2: m_shaders = loadGraphicsShader("TextureSampleBias.csl", outputLayoutNoDepth()); break;
                case 3: m_shaders = loadGraphicsShader("TextureSampleLod.csl", outputLayoutNoDepth()); break;
            }
        }

        void RenderingTest_TextureSample::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(outputLayoutNoDepth(), fb);

			DescriptorEntry desc[1];
            desc[0] = m_sampledImage;
            cmd.opBindDescriptor("TestParams"_id, desc);

            drawQuad(cmd, m_shaders, -0.8f, -0.8f, 1.6f, 1.6f);

            cmd.opEndPass();
        }

    } // test
} // rendering