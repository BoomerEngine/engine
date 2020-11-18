/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"
#include "renderingTestScene.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {
        /// test of the more advance compute shit - histogram computation
        class RenderingTest_ComputeHistogram : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ComputeHistogram, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float frameIndex, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            static const auto NUM_BUCKETS = 256;

            const ShaderLibrary* m_drawImage;
            const ShaderLibrary* m_drawHistogram;
            const ShaderLibrary* m_shaderFindMinMax;
            const ShaderLibrary* m_shaderComputeHistogram;
            const ShaderLibrary* m_shaderNormalizeHistogram;

            ImageView m_testImage;

            BufferView m_minMaxBuffer;
            BufferView m_redHistogramBuffer;
            BufferView m_greenHistogramBuffer;
            BufferView m_blueHistogramBuffer;

            void computeHistogram(command::CommandWriter& cmd, uint32_t componentType, const ImageView& view, const BufferView& outMinMax, const BufferView& outHistogram);
            void drawHistogram(command::CommandWriter& cmd, uint32_t componentType, const BufferView& minMax, const BufferView& histogram);
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ComputeHistogram);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2232);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(3);
        RTTI_END_TYPE();

        //---

        namespace
        {
            struct TestParams
            {
                ImageView TestImage;
            };

            struct ComputeHistogramConstants
            {
                uint32_t imageWidth = 0;
                uint32_t imageHeight = 0;
                uint32_t componentType = 0;
                uint32_t numberOfBuckets = 0;
                float verticalScale = 1.0f;
            };

            struct ComputeHistogramParams
            {
                ConstantsView Params;
                ImageView InputImage;
                BufferView MinMax;
                BufferView Buckets;
            };

        }

        void RenderingTest_ComputeHistogram::initialize()
        {
            // load test image
            if (subTestIndex() == 0)
                m_testImage = loadImage2D("lena.png", false, true, true).createSampledView(ObjectID::DefaultPointSampler());
            else if (subTestIndex() == 1)
                m_testImage = loadImage2D("sky_left.png", false, true, true).createSampledView(ObjectID::DefaultPointSampler());
            else if (subTestIndex() == 2)
                m_testImage = loadImage2D("checker_n.png", false, true, true).createSampledView(ObjectID::DefaultPointSampler());

            // load shaders
            m_drawImage = loadShader("TextureSample.csl");
            m_drawHistogram = loadShader("HistogramDraw.csl");
            m_shaderFindMinMax = loadShader("HistogramComputeMinMax.csl");
            m_shaderComputeHistogram = loadShader("HistogramCompute.csl");
            m_shaderNormalizeHistogram = loadShader("HistogramComputeMax.csl");

            m_minMaxBuffer = createStorageBuffer(sizeof(float) * 3);
            m_redHistogramBuffer = createStorageBuffer(sizeof(uint32_t) * NUM_BUCKETS);
            m_greenHistogramBuffer = createStorageBuffer(sizeof(uint32_t) * NUM_BUCKETS);
            m_blueHistogramBuffer = createStorageBuffer(sizeof(uint32_t) * NUM_BUCKETS);
        }

        void RenderingTest_ComputeHistogram::computeHistogram(command::CommandWriter& cmd, uint32_t componentType, const ImageView& view, const BufferView& outMinMax, const BufferView& outHistogram)
        {
            // bind params
            {
                ComputeHistogramConstants consts;
                consts.imageWidth = view.width();
                consts.imageHeight = view.height();
                consts.numberOfBuckets = NUM_BUCKETS;
                consts.verticalScale = 1.0f;
                consts.componentType = componentType;

                ComputeHistogramParams params;
                params.Params = cmd.opUploadConstants(consts);
                params.InputImage = view;
                params.MinMax = outMinMax;
                params.Buckets = outHistogram;

                cmd.opBindParametersInline("ComputeHistogramParams"_id, params);
            }

            // calculate min/max of values
            {
                const auto groupsX = (m_testImage.width() + 7) / 8;
                const auto groupsY = (m_testImage.height() + 7) / 8;
                cmd.opDispatch(m_shaderFindMinMax, groupsX, groupsY);
            }

            cmd.opGraphicsBarrier();

            // calculate the histogram
            {
                const auto groupsX = (m_testImage.width() + 7) / 8;
                const auto groupsY = (m_testImage.height() + 7) / 8;
                cmd.opDispatch(m_shaderComputeHistogram, groupsX, groupsY);
            }

            cmd.opGraphicsBarrier();

            // calculate the maximum value of stuff in histogram
            {
                const auto groupsX = (NUM_BUCKETS + 63) / 64;
                cmd.opDispatch(m_shaderNormalizeHistogram, groupsX);
            }

            cmd.opGraphicsBarrier();
        }

        void RenderingTest_ComputeHistogram::drawHistogram(command::CommandWriter& cmd, uint32_t componentType, const BufferView& minMax, const BufferView& histogram)
        {
            // bind params
            {
                ComputeHistogramConstants consts;
                consts.imageWidth = 512;
                consts.imageHeight = 512;
                consts.numberOfBuckets = NUM_BUCKETS;
                consts.verticalScale = 1.0f;
                consts.componentType = componentType;

                ComputeHistogramParams params;
                params.Params = cmd.opUploadConstants(consts);
                params.InputImage = m_testImage;// ImageView::DefaultBlack();
                params.MinMax = minMax;
                params.Buckets = histogram;

                cmd.opSetFillState(PolygonMode::Fill, 2.0f);
                cmd.opBindParametersInline("ComputeHistogramParams"_id, params);
                cmd.opSetPrimitiveType(PrimitiveTopology::LineStrip);
                cmd.opDraw(m_drawHistogram, 0, NUM_BUCKETS);
            }
        }

        void RenderingTest_ComputeHistogram::render(command::CommandWriter& cmd, float frameIndex, const ImageView& backBufferView, const ImageView& depth)
        {
            // draw
            {
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
                cmd.opBeingPass(fb);

                // draw preview
                {
                    TestParams tempParams;
                    tempParams.TestImage = m_testImage;
                    cmd.opBindParametersInline("TestParams"_id, tempParams);
                    drawQuad(cmd, m_drawImage, -0.8f, -0.8f, 0.6f, 0.6f);
                }

                // draw the histogram
                computeHistogram(cmd, 0, m_testImage, m_minMaxBuffer, m_redHistogramBuffer);
                computeHistogram(cmd, 1, m_testImage, m_minMaxBuffer, m_greenHistogramBuffer);
                computeHistogram(cmd, 2, m_testImage, m_minMaxBuffer, m_blueHistogramBuffer);
                drawHistogram(cmd, 0, m_minMaxBuffer, m_redHistogramBuffer);
                drawHistogram(cmd, 1, m_minMaxBuffer, m_greenHistogramBuffer);
                drawHistogram(cmd, 2, m_minMaxBuffer, m_blueHistogramBuffer);

                cmd.opEndPass();
            }
        }

    } // test
} // rendering