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
            virtual void render(command::CommandWriter& cmd, float frameIndex, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            const ShaderLibrary* m_shaderDraw;
            const ShaderLibrary* m_shaderPreview;
            const ShaderLibrary* m_shaderDraw2;

            BufferView m_vertexBuffer;
            ImageView m_depthBuffer;
            ImageView m_colorBuffer;
            ImageView m_resolvedColorBuffer;

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

        namespace
        {
            struct TestParams
            {
                ImageView TestImage;
                ImageView TestImageMS;
            };

            struct TestParams2
            {
                ImageView TestImageMS;
            };
        }


        void RenderingTest_MultiSampleBasic::initialize()
        {
            m_useDepth = subTestIndex() >= 5;
            m_showSamples = subTestIndex() >= 11;
            m_sampleCount = 1 << (subTestIndex() % 5);

            m_shaderDraw = loadShader("GenericGeometry.csl");
            m_shaderDraw2 = loadShader("AlphaToCoveragePreviewWithBorder.csl");

            if (m_showSamples)
            {
                m_shaderPreview = loadShader(base::TempString("MultiSampleTexturePreview{}.csl", subTestIndex() % 5));
            }
            else
            {
                m_shaderPreview = loadShader("MultiSampleTexturePreviewMix.csl");
            }

            // generate test geometry
            {
                base::Array<Simple3DVertex> vertices;
                PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);
                m_vertexBuffer = createVertexBuffer(vertices);
                m_vertexCount = vertices.size();
            }
        }

        void RenderingTest_MultiSampleBasic::render(command::CommandWriter& cmd, float frameIndex, const ImageView& backBufferView, const ImageView& depth)
        {
            // create depth render target
            if (m_depthBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::D24S8;
                info.width = backBufferView.width();
                info.height = backBufferView.height();
                info.numSamples = m_sampleCount;
                m_depthBuffer = createImage(info);
            }

            if (m_colorBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::RGBA8_UNORM;
                info.width = backBufferView.width();
                info.height = backBufferView.height();
                info.numSamples = m_sampleCount;
                m_colorBuffer = createImage(info);
            }

            if (m_resolvedColorBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::RGBA8_UNORM;
                info.width = backBufferView.width();
                info.height = backBufferView.height();
                m_resolvedColorBuffer = createImage(info);
            }

            FrameBuffer fb;
            fb.color[0].view(m_colorBuffer).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            if (m_useDepth)
                fb.depth.view(m_depthBuffer).clearDepth().clearStencil();

            {
                cmd.opBeingPass(fb);

                if (m_useDepth)
                    cmd.opSetDepthState();

                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
                cmd.opDraw(m_shaderDraw, 0, m_vertexCount);
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
                tempParams.TestImageMS = m_colorBuffer;
                cmd.opBindParametersInline("TestParams"_id, tempParams);
                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
                cmd.opDraw(m_shaderPreview, 0, 4);

                if (subTestIndex() < 10)
                {
                    TestParams2 tempParams;
                    tempParams.TestImageMS = m_resolvedColorBuffer.createSampledView(ObjectID::DefaultPointSampler());
                    cmd.opBindParametersInline("TestParams"_id, tempParams);
                    drawQuad(cmd, m_shaderDraw2, 0.2f, 0.2f, 0.8f, 0.8f, 0.45f, 0.45f, 0.55f, 0.55f);
                }

                cmd.opEndPass();              
            }

            //--
        }

    } // test
} // rendering
