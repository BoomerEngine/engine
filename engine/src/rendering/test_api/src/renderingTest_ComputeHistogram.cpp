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
#include "rendering/device/include/renderingPipeline.h"

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
            virtual void render(command::CommandWriter& cmd, float frameIndex, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

        private:
            static const auto NUM_BUCKETS = 256;

			ComputePipelineObjectPtr m_shaderFindRange;
			ComputePipelineObjectPtr m_shaderComputeHistogram;
			ComputePipelineObjectPtr m_shaderFindMax;
			ComputePipelineObjectPtr m_shaderNormalizeHistograms;

            GraphicsPipelineObjectPtr m_drawImage;
            GraphicsPipelineObjectPtr m_drawHistogram;

            ImageObjectPtr m_testImage;
			ImageReadOnlyViewPtr m_testImageSRV;
			ImageSampledViewPtr m_testImageSRVSampled;

			BufferObjectPtr m_redHistogramBuffer;
			BufferWritableViewPtr m_redHistogramBufferUAV;
			BufferViewPtr m_redHistogramBufferSRV;

			BufferObjectPtr m_greenHistogramBuffer;
			BufferWritableViewPtr m_greenHistogramBufferUAV;
			BufferViewPtr m_greenHistogramBufferSRV;

			BufferObjectPtr m_blueHistogramBuffer;
			BufferWritableViewPtr m_blueHistogramBufferUAV;
			BufferViewPtr m_blueHistogramBufferSRV;

            void computeHistogram(command::CommandWriter& cmd, uint32_t componentType, const ImageReadOnlyView* view, const BufferWritableView* outHistogram);
			void normalizeHistograms(command::CommandWriter& cmd, BufferWritableView* histograms[4]);
            void drawHistogram(command::CommandWriter& cmd, const BufferView* histogram, base::Color lineColor);
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ComputeHistogram);
            RTTI_METADATA(RenderingTestOrderMetadata).order(4000);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(3);
        RTTI_END_TYPE();

        //---

        void RenderingTest_ComputeHistogram::initialize()
        {
            // load test image
			if (subTestIndex() == 0)
				m_testImage = loadImage2D("lena.png", false, true);
            else if (subTestIndex() == 1)
                m_testImage = loadImage2D("sky_left.png", false, true);
            else if (subTestIndex() == 2)
                m_testImage = loadImage2D("checker_n.png", false, true);

            // load shaders
            m_drawImage = loadGraphicsShader("TextureSample.csl", outputLayoutNoDepth());
            m_drawHistogram = loadGraphicsShader("HistogramDraw.csl", outputLayoutNoDepth());
            m_shaderFindRange = loadComputeShader("HistogramFindRange.csl");
			m_shaderFindMax = loadComputeShader("HistogramFindMax.csl");
			m_shaderComputeHistogram = loadComputeShader("HistogramCompute.csl");
            m_shaderNormalizeHistograms = loadComputeShader("HistogramNormalize.csl");

			// resources
            m_redHistogramBuffer = createFormatBuffer(ImageFormat::R32_INT, sizeof(int) * (NUM_BUCKETS + 8), true);
            m_greenHistogramBuffer = createFormatBuffer(ImageFormat::R32_INT, sizeof(int) * (NUM_BUCKETS + 8), true);
            m_blueHistogramBuffer = createFormatBuffer(ImageFormat::R32_INT, sizeof(int) * (NUM_BUCKETS + 8), true);

			// views
			m_testImageSRV = m_testImage->createReadOnlyView();
			m_testImageSRVSampled = m_testImage->createSampledView();

			m_redHistogramBufferUAV = m_redHistogramBuffer->createWritableView(ImageFormat::R32_INT);
			m_redHistogramBufferSRV = m_redHistogramBuffer->createView(ImageFormat::R32_INT);
			m_greenHistogramBufferUAV = m_greenHistogramBuffer->createWritableView(ImageFormat::R32_INT);
			m_greenHistogramBufferSRV = m_greenHistogramBuffer->createView(ImageFormat::R32_INT);
			m_blueHistogramBufferUAV = m_blueHistogramBuffer->createWritableView(ImageFormat::R32_INT);
			m_blueHistogramBufferSRV = m_blueHistogramBuffer->createView(ImageFormat::R32_INT);
        }

        void RenderingTest_ComputeHistogram::computeHistogram(command::CommandWriter& cmd, uint32_t componentType, const ImageReadOnlyView* view, const BufferWritableView* outHistogram)
        {
			// make sure UAV writes are visible
			cmd.opTransitionFlushUAV(outHistogram);

            // calculate min/max of values
            {
				struct
				{
					uint32_t imageWidth = 0;
					uint32_t imageHeight = 0;
					uint32_t componentType = 0;
					uint32_t numberOfBuckets = 0;
					float minAllowedRange = 0.0f;
					float maxAllowedRange = 1.0f;
				} consts;

				consts.imageWidth = view->image()->width();
				consts.imageHeight = view->image()->height();
				consts.numberOfBuckets = NUM_BUCKETS;
				consts.componentType = componentType;

				DescriptorEntry desc[3];
				desc[0].constants(consts);
				desc[1] = view;
				desc[2] = outHistogram;
				cmd.opBindDescriptor("HistogramDesc"_id, desc);
                cmd.opDispatchThreads(m_shaderFindRange, consts.imageWidth, consts.imageHeight);
            }

            // calculate the histogram
            {
				struct
				{
					uint32_t imageWidth = 0;
					uint32_t imageHeight = 0;
					uint32_t componentType = 0;
					uint32_t numberOfBuckets = 0;
				} consts;

				consts.imageWidth = view->image()->width();
				consts.imageHeight = view->image()->height();
				consts.numberOfBuckets = NUM_BUCKETS;
				consts.componentType = componentType;

				DescriptorEntry desc[3];
				desc[0].constants(consts);
				desc[1] = view;
				desc[2] = outHistogram;
				cmd.opBindDescriptor("HistogramDesc"_id, desc);
				cmd.opDispatchThreads(m_shaderComputeHistogram, consts.imageWidth, consts.imageHeight);
            }

			// make sure UAV writes are visible
			cmd.opTransitionFlushUAV(outHistogram);

            // calculate the maximum value of stuff in histogram
            {
				struct
				{
					uint32_t numberOfBuckets = 0;
				} consts;

				consts.numberOfBuckets = NUM_BUCKETS;

				DescriptorEntry desc[2];
				desc[0].constants(consts);
				desc[1] = outHistogram;
				cmd.opBindDescriptor("HistogramDesc"_id, desc);
                cmd.opDispatchThreads(m_shaderFindMax, NUM_BUCKETS);
            }

			// make sure UAV writes are visible
			cmd.opTransitionFlushUAV(outHistogram);
        }

		void RenderingTest_ComputeHistogram::normalizeHistograms(command::CommandWriter& cmd, BufferWritableView* histograms[4])
		{
			struct
			{
				uint32_t numberOfBuckets = 0;
			} consts;

			consts.numberOfBuckets = NUM_BUCKETS;

			DescriptorEntry desc[5];
			desc[0].constants(consts);
			desc[1] = histograms[0];
			desc[2] = histograms[1];
			desc[3] = histograms[2];
			desc[4] = histograms[3];
			cmd.opBindDescriptor("HistogramDesc"_id, desc);
			cmd.opDispatchThreads(m_shaderNormalizeHistograms, 1);

			cmd.opTransitionFlushUAV(histograms[0]);
			cmd.opTransitionFlushUAV(histograms[1]);
			cmd.opTransitionFlushUAV(histograms[2]);
			cmd.opTransitionFlushUAV(histograms[3]);
		}

        void RenderingTest_ComputeHistogram::drawHistogram(command::CommandWriter& cmd, const BufferView* histogram, base::Color lineColor)
        {
			struct
			{
				uint32_t numberOfBuckets = 0;
				uint32_t padding0;
				uint32_t padding1;
				uint32_t padding2;

				base::Vector4 color;

				float offsetX = -0.9f;
				float offsetY = 0.8f;
				float sizeX = 1.8f;
				float sizeY = -0.8f;
			} consts;

			consts.numberOfBuckets = NUM_BUCKETS;
			consts.color = lineColor.toVectorLinear();

			DescriptorEntry desc[2];
			desc[0].constants(consts);
			desc[1] = histogram;
			cmd.opBindDescriptor("DrawHistogramDesc"_id, desc);
			cmd.opDraw(m_drawHistogram, 0, NUM_BUCKETS);
        }

        void RenderingTest_ComputeHistogram::render(command::CommandWriter& cmd, float frameIndex, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
			// clear buffers
			cmd.opClearWritableBuffer(m_redHistogramBufferUAV);
			cmd.opClearWritableBuffer(m_greenHistogramBufferUAV);
			cmd.opClearWritableBuffer(m_blueHistogramBufferUAV);

			// compute the histograms
			computeHistogram(cmd, 0, m_testImageSRV, m_redHistogramBufferUAV);
			computeHistogram(cmd, 1, m_testImageSRV, m_greenHistogramBufferUAV);
			computeHistogram(cmd, 2, m_testImageSRV, m_blueHistogramBufferUAV);

			// normalize histogram
			{
				BufferWritableView* histograms[4];
				histograms[0] = m_redHistogramBufferUAV;
				histograms[1] = m_greenHistogramBufferUAV;
				histograms[2] = m_blueHistogramBufferUAV;
				histograms[3] = m_blueHistogramBufferUAV;
				normalizeHistograms(cmd, histograms);
			}

			// transition data
			cmd.opTransitionLayout(m_redHistogramBuffer, ResourceLayout::UAV, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_greenHistogramBuffer, ResourceLayout::UAV, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_blueHistogramBuffer, ResourceLayout::UAV, ResourceLayout::ShaderResource);

            // draw
            {
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
                cmd.opBeingPass(outputLayoutNoDepth(), fb);

                // draw preview
                {
					DescriptorEntry desc[1];
					desc[0] = m_testImageSRVSampled;
                    cmd.opBindDescriptor("TestParams"_id, desc);
                    drawQuad(cmd, m_drawImage, -0.8f, -0.8f, 1.2f, 1.2f);
                }

				// draw the histograms
				drawHistogram(cmd, m_redHistogramBufferSRV, base::Color::RED);
				drawHistogram(cmd, m_greenHistogramBufferSRV, base::Color::GREEN);
				drawHistogram(cmd, m_blueHistogramBufferSRV, base::Color::BLUE);

                cmd.opEndPass();
            }
        }

    } // test
} // rendering