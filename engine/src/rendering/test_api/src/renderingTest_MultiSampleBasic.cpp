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

#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"

namespace rendering
{
    namespace test
    {
        /// basic multisample test
        class RenderingTest_MultiSampleBasic : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_MultiSampleBasic, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float frameIndex, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

        private:
			GraphicsPipelineObjectPtr m_shaderDraw;
			GraphicsPipelineObjectPtr m_shaderPreview;
			GraphicsPipelineObjectPtr m_shaderDraw2;

            BufferObjectPtr m_vertexBuffer;

            ImageObjectPtr m_depthBuffer;
			RenderTargetViewPtr m_depthBufferRTV;
			ImageObjectPtr m_colorBuffer;
			RenderTargetViewPtr m_colorBufferRTV;
			ImageSampledViewPtr m_colorBufferSRV;

			GraphicsPassLayoutObjectPtr m_colorBufferPassLayout;

			ImageObjectPtr m_resolvedColorBuffer;
			ImageSampledViewPtr m_resolvedColorBufferSRV;

            uint32_t m_vertexCount = 0;
            bool m_useDepth = false;
            bool m_showSamples = false;
            uint8_t m_sampleCount = 1;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_MultiSampleBasic);
            RTTI_METADATA(RenderingTestOrderMetadata).order(1300);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(15);
        RTTI_END_TYPE();

        //---       

        static float RandOne()
        {
            return (float)rand() / (float)RAND_MAX;
        }

        static float RandRange(float min, float max)
        {
            return min + (max - min) * RandOne();
        }

        static void PrepareTestGeometry(float x, float y, float w, float h, base::Array<Simple3DVertex>& outVertices)
        {
            static auto NUM_TRIS = 256;

            static const float PHASE_A = 0.0f;
            static const float PHASE_B = DEG2RAD * 120.0f;
            static const float PHASE_C = DEG2RAD * 240.0f;

            srand(0);
            for (uint32_t i = 0; i < 50; ++i)
            {
                auto radius = 0.05f + 0.4f * RandOne();
                auto color = base::Color::FromVectorLinear(base::Vector4(RandRange(0.2f, 1.0f), RandRange(0.2f, 1.0f), RandRange(0.2f, 1.0f), 1.0f));

                auto ox = RandRange(-0.95f + radius, 0.95f - radius);
                auto oy = RandRange(-0.95f + radius, 0.95f - radius);

                auto phase = RandRange(0, 7.0f);
                auto ax = ox + radius * cos(PHASE_A + phase);
                auto ay = oy + radius * sin(PHASE_A + phase);
                auto az = RandRange(0.0f, 1.0f);

                auto bx = ox + radius * cos(PHASE_B + phase);
                auto by = oy + radius * sin(PHASE_B + phase);
                auto bz = RandRange(0.0f, 1.0f);

                auto cx = ox + radius * cos(PHASE_C + phase);
                auto cy = oy + radius * sin(PHASE_C + phase);
                auto cz = RandRange(0.0f, 1.0f);

                outVertices.emplaceBack(ax, ay, az, 0.0f, 0.0f, color);
                outVertices.emplaceBack(bx, by, bz, 0.0f, 0.0f, color);
                outVertices.emplaceBack(cx, cy, cz, 0.0f, 0.0f, color);
            }
        }

        void RenderingTest_MultiSampleBasic::initialize()
        {
            m_useDepth = subTestIndex() >= 5;
            m_showSamples = subTestIndex() >= 11;
            m_sampleCount = 1 << (subTestIndex() % 5);

			{
				GraphicsPassLayoutSetup setup;
				setup.samples = m_sampleCount;
				setup.color[0].format = ImageFormat::RGBA8_UNORM;
				if (m_useDepth)
					setup.depth.format = ImageFormat::D24S8;

				m_colorBufferPassLayout = createPassLayout(setup);
			}

			if (m_useDepth)
			{
				GraphicsRenderStatesSetup setup;
				setup.depth(true);
				setup.depthWrite(true);
				setup.depthFunc(CompareOp::LessEqual);

				m_shaderDraw = loadGraphicsShader("GenericGeometry.csl", m_colorBufferPassLayout, &setup);
			}
			else
			{
				GraphicsRenderStatesSetup setup;
				setup.depth(false);

				m_shaderDraw = loadGraphicsShader("GenericGeometry.csl", m_colorBufferPassLayout, &setup);
			}

            m_shaderDraw2 = loadGraphicsShader("AlphaToCoveragePreviewWithBorder.csl", outputLayoutNoDepth());

            if (m_showSamples)
                m_shaderPreview = loadGraphicsShader(base::TempString("MultiSampleTexturePreview{}.csl", subTestIndex() % 5), outputLayoutNoDepth());
            else
                m_shaderPreview = loadGraphicsShader("MultiSampleTexturePreviewMix.csl", outputLayoutNoDepth());

            // generate test geometry
            {
                base::Array<Simple3DVertex> vertices;
                PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);
                m_vertexBuffer = createVertexBuffer(vertices);
                m_vertexCount = vertices.size();
            }
        }

        void RenderingTest_MultiSampleBasic::render(command::CommandWriter& cmd, float frameIndex, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
            // create depth render target
            if (m_depthBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::D24S8;
                info.width = backBufferView->width();
                info.height = backBufferView->height();
                info.numSamples = m_sampleCount;
                m_depthBuffer = createImage(info);
				m_depthBufferRTV = m_depthBuffer->createRenderTargetView();
            }

            if (m_colorBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::RGBA8_UNORM;
                info.width = backBufferView->width();
                info.height = backBufferView->height();
                info.numSamples = m_sampleCount;
                m_colorBuffer = createImage(info);
				m_colorBufferRTV = m_colorBuffer->createRenderTargetView();
				m_colorBufferSRV = m_colorBuffer->createSampledView();
            }

            if (m_resolvedColorBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::RGBA8_UNORM;
                info.width = backBufferView->width();
                info.height = backBufferView->height();
                m_resolvedColorBuffer = createImage(info);
				m_resolvedColorBufferSRV = m_resolvedColorBuffer->createSampledView();
            }

            FrameBuffer fb;
            fb.color[0].view(m_colorBufferRTV).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            if (m_useDepth)
                fb.depth.view(m_depthBufferRTV).clearDepth().clearStencil();

            {
                cmd.opBeingPass(m_colorBufferPassLayout, fb );

                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
                cmd.opDraw(m_shaderDraw, 0, m_vertexCount);
                cmd.opEndPass();
            }

			//--

			cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::RenderTarget, ResourceLayout::ResolveSource);
			cmd.opTransitionLayout(m_resolvedColorBuffer, ResourceLayout::ShaderResource, ResourceLayout::ResolveDest);
			cmd.opResolve(m_colorBuffer, m_resolvedColorBuffer);
			cmd.opTransitionLayout(m_resolvedColorBuffer, ResourceLayout::ResolveDest, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::ResolveSource, ResourceLayout::ShaderResource);

			//--

            {
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
                cmd.opBeingPass(outputLayoutNoDepth(), fb);

				if (m_showSamples)
				{
					DescriptorEntry desc[2];
					desc[0] = m_resolvedColorBufferSRV;
					desc[1] = m_colorBufferSRV;
					cmd.opBindDescriptor("TestParams"_id, desc);
					cmd.opDraw(m_shaderPreview, 0, 4);
				}
				else
				{
					DescriptorEntry desc[1];
					desc[0] = m_resolvedColorBufferSRV;
					cmd.opBindDescriptor("TestParams"_id, desc);
					cmd.opDraw(m_shaderPreview, 0, 4);
				}

                if (subTestIndex() < 10)
                {
					DescriptorEntry desc[1];
					desc[0] = m_resolvedColorBufferSRV;
                    cmd.opBindDescriptor("TestParams"_id, desc);
                    drawQuad(cmd, m_shaderDraw2, 0.2f, 0.2f, 0.8f, 0.8f, 0.45f, 0.45f, 0.55f, 0.55f);
                }

                cmd.opEndPass();              
            }

            //--
        }

    } // test
} // rendering
