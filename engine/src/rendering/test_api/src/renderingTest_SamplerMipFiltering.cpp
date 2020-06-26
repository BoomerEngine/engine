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
        /// test of a mipmap filtering mode
        class RenderingTest_SamplerMipMapFiltering : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_SamplerMipMapFiltering, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            const ShaderLibrary* m_shaders;
            BufferView m_vertexBuffer;
            ImageView m_sampledImage;

            ObjectID m_samplers[4];
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_SamplerMipMapFiltering);
            RTTI_METADATA(RenderingTestOrderMetadata).order(1030);
        RTTI_END_TYPE();

        //---       }

        static void PrepareTestGeometry(base::Array<Simple3DVertex>& outVertices)
        {
            float zNear = 0.0f;
            float zFar = 1.0f;

            outVertices.pushBack(Simple3DVertex(-1.0f, 1.0f, zNear, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(1.0f, 1.0f, zNear, 1.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(1.0f, -1.0f, zFar, 1.0f, 1.0f));

            outVertices.pushBack(Simple3DVertex(-1.0f, 1.0f, zNear, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(1.0f, -1.0f, zFar, 1.0f, 1.0f));
            outVertices.pushBack(Simple3DVertex(-1.0f, -1.0f, zFar, 0.0f, 1.0f));
        }

        void RenderingTest_SamplerMipMapFiltering::initialize()
        {
            // generate test geometry
            base::Array<Simple3DVertex> vertices;
            PrepareTestGeometry(vertices);

            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = vertices.dataSize();

                auto sourceData = CreateSourceData(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
            }

            // create the test image
            m_sampledImage = createChecker2D(256, 4);

            // create the no-mip sampler
            {
                SamplerState info;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.minFilter = FilterMode::Linear;
                info.magFilter = FilterMode::Linear;
                info.maxLod = 0.0f;
                m_samplers[0] = createSampler(info);
            }

            // create the bilinear mip sampler
            {
                SamplerState info;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.minFilter = FilterMode::Linear;
                info.magFilter = FilterMode::Linear;
                m_samplers[1] = createSampler(info);
            }

            // create the trilinear mip sampler
            {
                SamplerState info;
                info.mipmapMode = MipmapFilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.magFilter = FilterMode::Linear;
                m_samplers[2] = createSampler(info);
            }

            // create the aniso mip sampler
            {
                SamplerState info;
                info.mipmapMode = MipmapFilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.magFilter = FilterMode::Linear;
                info.maxAnisotropy = 16;
                m_samplers[3] = createSampler(info);
            }

            m_shaders = loadShader("SamplerMipMapFiltering.csl");
        }

        namespace
        {
            struct TestConsts
            {
                float OffsetZ;
                float MaxZ;
            };

            struct TestParams
            {
                ConstantsView Consts;
                ImageView TestTexture;
            };
        }

        void RenderingTest_SamplerMipMapFiltering::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);

            {
                uint32_t numSteps = 4;
                uint32_t viewportWidth = backBufferView.width() / numSteps;
                uint32_t viewportHeight = backBufferView.height() / numSteps;

                for (uint32_t y = 0; y < numSteps; ++y)
                {
                    TestParams tempParams;
                    tempParams.TestTexture = m_sampledImage.createSampledView(m_samplers[y]);

                    static const float zRanges[4] = { 1.0f, 5.0f, 10.0f, 50.0f };
                    for (uint32_t x = 0; x < numSteps; ++x)
                    {
                        TestConsts consts;
                        consts.OffsetZ = 1.0f;
                        consts.MaxZ = zRanges[x];

                        tempParams.Consts = cmd.opUploadConstants(consts);
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