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

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingCommandWriter.h"

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
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            ImageView m_sampledImage;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_TextureSample);
            RTTI_METADATA(RenderingTestOrderMetadata).order(710);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(4);
        RTTI_END_TYPE();

        //---

        namespace
        {
            struct TestParams
            {
                ImageView TestImage;
            };
        }

        void RenderingTest_TextureSample::initialize()
        {
            m_sampledImage = loadImage2D("lena.png", true).createSampledView(ObjectID::DefaultTrilinearSampler());

            switch (subTestIndex())
            {
                case 0: m_shaders = loadShader("TextureSample.csl"); break;
                case 1: m_shaders = loadShader("TextureSampleOffset.csl"); break;
                case 2: m_shaders = loadShader("TextureSampleBias.csl"); break;
                case 3: m_shaders = loadShader("TextureSampleLod.csl"); break;
            }
        }

        void RenderingTest_TextureSample::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);

            TestParams tempParams;
            tempParams.TestImage = m_sampledImage;
            cmd.opBindParametersInline("TestParams"_id, tempParams);

            DrawQuad(cmd, m_shaders, -0.8f, -0.8f, 1.6f, 1.6f);

            cmd.opEndPass();
        }

    } // test
} // rendering