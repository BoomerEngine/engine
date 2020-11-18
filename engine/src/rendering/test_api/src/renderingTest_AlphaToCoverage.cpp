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

namespace rendering
{
    namespace test
    {
        /// alpha to coverage test
        class RenderingTest_AlphaToCoverage : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_AlphaToCoverage, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

            const ShaderLibrary* m_shaderGenerate;
            const ShaderLibrary* m_shaderDraw;
            const ShaderLibrary* m_shaderDraw2;

            ImageView m_staticImage;
            ImageView m_colorBuffer;
            ImageView m_resolvedColorBuffer;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_AlphaToCoverage);
            RTTI_METADATA(RenderingTestOrderMetadata).order(1600);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(10);
        RTTI_END_TYPE();

        //---       

        namespace
        {
            struct TestParams
            {
                ImageView TestImage;
            };
        }

        void RenderingTest_AlphaToCoverage::initialize()
        {
            m_staticImage = loadImage2D("mask.png");

            m_shaderDraw = loadShader("AlphaToCoveragePreview.csl");
            m_shaderDraw2 = loadShader("AlphaToCoveragePreviewWithBorder.csl");
            m_shaderGenerate = loadShader("AlphaToCoverageGenerate.csl");

            //auto numSamples = 1 << subTestIndex();
            //m_shaders = loadShader(base::TempString("AlphaToCoverage.csl:NUM_SAMPLES={}", numSamples));
        }

        void RenderingTest_AlphaToCoverage::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            if (m_colorBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.format = rendering::ImageFormat::RGBA8_UNORM;
                info.width = backBufferView.width();
                info.height = backBufferView.height();
                info.numSamples = 1 << (subTestIndex()/2);
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

                TestParams tempParams;
                tempParams.TestImage = m_staticImage;
                cmd.opBindParametersInline("TestParams"_id, tempParams);

                {
                    MultisampleState state;
                    state.alphaToCoverageEnable = true;
                    state.alphaToCoverageDitherEnable = (subTestIndex() & 1);
                    cmd.opSetMultisampleState(state);
                }
                
                float w = 0.8f;
                float h = 0.8f;
                float r = 0.4f;
                for (uint32_t i = 0; i < 12; ++i)
                {
                    float s1 = 1.0f + i * 0.025352f;
                    float o1 = 0.1212f + i * 0.74747f;

                    float s2 = 1.1f + i * 0.029222f;
                    float o2 = 0.17871f + i * 0.123123f;

                    float x = w * sin(o1 + time * s1);
                    float y = h * sin(o2 + time * s2);

                    auto color = base::Color::RED;
                    if ((i % 3) == 1) 
                        color = base::Color::GREEN;
                    if ((i % 3) == 2)
                        color = base::Color::BLUE;
                    DrawQuad(cmd, m_shaderGenerate, x - r * 0.5f, y - r * 0.5f, r, r, 0,0,1,1,color);
                }

                {
                    MultisampleState state;
                    cmd.opSetMultisampleState(state);
                }

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

                {
                    TestParams tempParams;
                    tempParams.TestImage = m_resolvedColorBuffer;
                    cmd.opBindParametersInline("TestParams"_id, tempParams);
                    DrawQuad(cmd, m_shaderDraw, -1.0f, -1.0f, 2.0f, 2.0f);
                }

                {
                    TestParams tempParams;
                    tempParams.TestImage = m_resolvedColorBuffer.createSampledView(ObjectID::DefaultPointSampler());
                    cmd.opBindParametersInline("TestParams"_id, tempParams);
                    DrawQuad(cmd, m_shaderDraw2, 0.2f, 0.2f, 0.8f, 0.8f, 0.45f, 0.45f, 0.55f, 0.55f);
                }

                cmd.opEndPass();
            }
        }

        //--

    } // test
} // rendering
