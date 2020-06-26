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

#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"

namespace rendering
{
    namespace test
    {

        /// depth compare test
        class RenderingTest_DepthCompareSampler : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_DepthCompareSampler, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView) override final;

        private:
            const ShaderLibrary* m_shaderTest;
            const ShaderLibrary* m_shaderPreview;

            BufferView m_vertexBuffer;
            ImageView m_depthBuffer;
            uint32_t m_vertexCount;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_DepthCompareSampler);
            RTTI_METADATA(RenderingTestOrderMetadata).order(1100);
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
                ConstantsView Constants;
                ImageView TestImage;
            };
        };

        void RenderingTest_DepthCompareSampler::initialize()
        {
            // generate test geometry
            base::Array<Simple3DVertex> vertices;
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);

            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = vertices.dataSize();

                auto sourceData = CreateSourceData(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
                m_vertexCount = vertices.size();
            }

            m_shaderTest = loadShader("DepthCompareSamplerDraw.csl");
            m_shaderPreview = loadShader("DepthCompareSamplerPreview.csl");
        }
        
        void RenderingTest_DepthCompareSampler::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView)
        {
            // create depth render target
            if (m_depthBuffer.empty())
            {
                ImageCreationInfo info;
                info.allowShaderReads = true;
                info.allowRenderTarget = true;
                info.format = rendering::ImageFormat::D24S8;
                info.width = backBufferView.width();
                info.height = backBufferView.height();
                m_depthBuffer = createImage(info);
            }

            // draw the triangles
            {
                FrameBuffer fb;
                fb.depth.view(m_depthBuffer).clearDepth(1.0f).clearStencil(0);

                cmd.opBeingPass(fb);
                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
                cmd.opSetDepthState();
                cmd.opDraw(m_shaderTest, 0, m_vertexCount);
                cmd.opEndPass();
            }

            // we will wait for the buffer to be generated
            cmd.opGraphicsBarrier();

            // draw the depth compare test
            {
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

                cmd.opBeingPass(fb);

                auto depthCompareSampler = rendering::ObjectID::DefaultDepthPointSampler();

                float zRef = 0.5f + 0.5f * sinf(time);

                TestParams tempParams;
                tempParams.TestImage = m_depthBuffer.createSampledView(depthCompareSampler);
                tempParams.Constants = cmd.opUploadConstants(zRef);
                cmd.opBindParametersInline("TestParams"_id, tempParams);

                DrawQuad(cmd, m_shaderPreview, -0.9f, -0.9f, 1.8f, 1.8f);
                cmd.opEndPass();
            }

            //--
        }

    } // test
} // rendering
