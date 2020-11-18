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
        /// test of a sampler lod bias
        class RenderingTest_SamplerLodBias : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_SamplerLodBias, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            BufferView m_vertexBuffer;
            ImageView m_sampledImage;
            ObjectID m_sampler;
            const ShaderLibrary* m_shaders;
            MipmapFilterMode m_filterMode = MipmapFilterMode::Nearest;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_SamplerLodBias);
            RTTI_METADATA(RenderingTestOrderMetadata).order(1010);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(2);
        RTTI_END_TYPE();

        //---

        static void AddQuad(float x, float y, float size, base::Array<Simple3DVertex>& outVertices)
        {
        }

        static void PrepareTestGeometry(base::Array<Simple3DVertex>& outVertices)
        {
            float zNear = 1.0f;
            float zFar = 100.0f;

            outVertices.pushBack(Simple3DVertex(-1.0f, 1.0f, zNear, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(1.0f, 1.0f, zNear, 1.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(1.0f, -1.0f, zFar, 1.0f, 1.0f));

            outVertices.pushBack(Simple3DVertex(-1.0f, 1.0f, zNear, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(1.0f, -1.0f, zFar, 1.0f, 1.0f));
            outVertices.pushBack(Simple3DVertex(-1.0f, -1.0f, zFar, 0.0f, 1.0f));
        }

        void RenderingTest_SamplerLodBias::initialize()
        {
            {
                base::Array<Simple3DVertex> vertices;
                PrepareTestGeometry(vertices);
                m_vertexBuffer = createVertexBuffer(vertices);
            }

            m_sampledImage = createMipmapTest2D(256);

            m_filterMode = subTestIndex() ? MipmapFilterMode::Linear : MipmapFilterMode::Nearest;
            m_shaders = loadShader("SamplerFiltering.csl");
        }

        namespace
        {
            struct TestParams
            {
                ImageView TestTexture;
            };
        }

        void RenderingTest_SamplerLodBias::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);

            {
                uint32_t numSteps = 4;
                float lodStep = 0.5f;
                float minLod = -2.0f;
                uint32_t viewportWidth = backBufferView.width() / numSteps;
                uint32_t viewportHeight = backBufferView.height() / numSteps;

                for (uint32_t y = 0; y < numSteps; ++y)
                {
                    for (uint32_t x = 0; x < numSteps; ++x)
                    {
                        SamplerState info;
                        info.mipmapMode = m_filterMode;
                        info.mipLodBias = minLod + ((x + y * numSteps) * lodStep);
                        auto sampler = device()->createSampler(info);

                        TestParams tempParams;
                        tempParams.TestTexture = m_sampledImage.createSampledView(sampler->id());
                        cmd.opBindParametersInline("TestParams"_id, tempParams);

                        cmd.opSetViewportRect(0, x * viewportWidth, y * viewportHeight, viewportWidth, viewportHeight);
                        cmd.opDraw(m_shaders, 0, m_vertexBuffer.size() / sizeof(Simple3DVertex));
                    }
                }
            }

            cmd.opEndPass();            
        }

    } // test
} // rendering