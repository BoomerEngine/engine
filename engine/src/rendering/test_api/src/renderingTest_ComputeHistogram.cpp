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
            const ShaderLibrary* m_drawImage;
            const ShaderLibrary* m_drawHistogram;
            const ShaderLibrary* m_shaderFindMinMax;
            const ShaderLibrary* m_shaderComputeHistogram;
            const ShaderLibrary* m_shaderNormalizeHistogram;

            ImageView m_testImage;

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

        static const auto NumBuckets = 256;

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
        }

        void RenderingTest_ComputeHistogram::computeHistogram(command::CommandWriter& cmd, uint32_t componentType, const ImageView& view, const BufferView& outMinMax, const BufferView& outHistogram)
        {
            // bind params
            {
                ComputeHistogramConstants consts;
                consts.imageWidth = view.width();
                consts.imageHeight = view.height();
                consts.numberOfBuckets = NumBuckets;
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
                const auto groupsX = (NumBuckets + 63) / 64;
                cmd.opDispatch(m_shaderNormalizeHistogram, groupsX);
            }

            cmd.opGraphicsBarrier();
        }

        void RenderingTest_ComputeHistogram::drawHistogram(command::CommandWriter& cmd, uint32_t componentType, const BufferView& minMax, const BufferView& histogram)
        {
            const auto numBuckets = 256;

            // bind params
            {
                ComputeHistogramConstants consts;
                consts.imageWidth = 512;
                consts.imageHeight = 512;
                consts.numberOfBuckets = NumBuckets;
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
                cmd.opDraw(m_drawHistogram, 0, numBuckets);
            }
        }

        void RenderingTest_ComputeHistogram::render(command::CommandWriter& cmd, float frameIndex, const ImageView& backBufferView, const ImageView& depth)
        {
            TransientBufferView minMaxBuffer(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadWrite, sizeof(float) * 3);
            cmd.opAllocTransientBuffer(minMaxBuffer);
            TransientBufferView redHistogramBuffer(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadWrite, sizeof(uint32_t) * NumBuckets);
            TransientBufferView greenHistogramBuffer(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadWrite, sizeof(uint32_t) * NumBuckets);
            TransientBufferView blueHistogramBuffer(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadWrite, sizeof(uint32_t) * NumBuckets);
            cmd.opAllocTransientBuffer(redHistogramBuffer);
            cmd.opAllocTransientBuffer(greenHistogramBuffer);
            cmd.opAllocTransientBuffer(blueHistogramBuffer);

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
                    DrawQuad(cmd, m_drawImage, -0.8f, -0.8f, 0.6f, 0.6f);
                }

                // draw the histogram
                computeHistogram(cmd, 0, m_testImage, minMaxBuffer, redHistogramBuffer);
                computeHistogram(cmd, 1, m_testImage, minMaxBuffer, greenHistogramBuffer);
                computeHistogram(cmd, 2, m_testImage, minMaxBuffer, blueHistogramBuffer);
                drawHistogram(cmd, 0, minMaxBuffer, redHistogramBuffer);
                drawHistogram(cmd, 1, minMaxBuffer, greenHistogramBuffer);
                drawHistogram(cmd, 2, minMaxBuffer, blueHistogramBuffer);

                cmd.opEndPass();
            }
        }

    } // test
} // rendering