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

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"

#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"

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
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            const ShaderLibrary* m_shaderDraw;
            const ShaderLibrary* m_shaderGenerate;

            uint8_t m_sampleCount = 1;
            ImageView m_staticImage;

            ImageView m_colorBuffer;
            ImageView m_resolvedColorBuffer;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_SampleMask);
            RTTI_METADATA(RenderingTestOrderMetadata).order(1500);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(5);
        RTTI_END_TYPE();

        //---       

        namespace
        {
            struct TestParams
            {
                ImageView TestImage;
            };
        }

        void RenderingTest_SampleMask::initialize()
        {
            m_sampleCount = 1 << subTestIndex();

            m_shaderDraw = loadShader("SampleMaskPreview.csl");
            m_shaderGenerate = loadShader(base::TempString("SampleMaskGenerate{}.csl", subTestIndex()));
        }

        void RenderingTest_SampleMask::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            if (m_colorBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.format = rendering::ImageFormat::RGBA8_UNORM;
                info.width = backBufferView.width();
                info.height = backBufferView.height();
                info.numSamples = m_sampleCount;
                m_colorBuffer = createImage(info);
            }

            if (m_resolvedColorBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::RGBA8_UNORM;
                info.width = backBufferView.width();
                info.height = backBufferView.height();
                m_resolvedColorBuffer = createImage(info);
            }

            // draw triangles
            {
                FrameBuffer fb;
                fb.color[0].view(m_colorBuffer).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

                cmd.opBeingPass(fb);

                DrawQuad(cmd, m_shaderGenerate, -0.9f, -0.9f, 1.8f, 1.8f);

                cmd.opEndPass();
            }

            //--

            cmd.opGraphicsBarrier();
            cmd.opResolve(m_colorBuffer, m_resolvedColorBuffer);

            //--

            {
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
                cmd.opBeingPass(fb);

                TestParams tempParams;
                tempParams.TestImage = m_resolvedColorBuffer;
                cmd.opBindParametersInline("TestParams"_id, tempParams);

                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
                cmd.opDraw(m_shaderDraw, 0, 4);
                cmd.opEndPass();
            }
        }

    } // test
} // rendering
