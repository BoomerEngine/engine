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
        /// test of the compute shader access to storage image
        class RenderingTest_ComputeFillTexture2D : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ComputeFillTexture2D, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            const ShaderLibrary* m_shaderGenerate;
            const ShaderLibrary* m_shaderTest;

            BufferView m_vertexBuffer;
            ImageView m_imageWritable;

            uint32_t m_vertexCount;

            uint16_t SIDE_RESOLUTION = 512;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ComputeFillTexture2D);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2210);
        RTTI_END_TYPE();

        //---       

        namespace
        {
            struct TestConsts
            {
                base::Vector2 ZoomOffset;
                base::Vector2 ZoomScale;
                uint32_t SideCount;
                uint32_t MaxIterations;
            };

            struct TestParamsWrite
            {
                ConstantsView Params;
                ImageView Colors;
            };

            struct TestParamsRead
            {
                ImageView Colors;
            };
        };

        static void PrepareTestGeometry(float x, float y, float w, float h, base::Array<Simple3DVertex>& outVertices)
        {
            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y, 0.5f, 1.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));

            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));
            outVertices.pushBack(Simple3DVertex(x, y + h, 0.5f, 0.0f, 1.0f));
        }

        void RenderingTest_ComputeFillTexture2D::initialize()
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
            }

            // create image
            ImageCreationInfo storageImageSetup;
            storageImageSetup.format = ImageFormat::RGBA8_UNORM;
            storageImageSetup.width = SIDE_RESOLUTION;
            storageImageSetup.height = SIDE_RESOLUTION;
            storageImageSetup.view = ImageViewType::View2D;
            storageImageSetup.allowShaderReads = true;
            storageImageSetup.allowUAV = true;
            m_imageWritable = createImage(storageImageSetup);

            m_shaderGenerate = loadShader("ComputeFillTexture2DGenerate.csl");
            m_shaderTest = loadShader("ComputeFillTexture2DText.csl");
        }

        void RenderingTest_ComputeFillTexture2D::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            // transition the data to reading format
            cmd.opImageLayoutBarrier(m_imageWritable, ImageLayout::ShaderReadWrite);

            // upload consts
            float linear = std::fmod(time * 0.2f, 1.0f); // 10s
            float logZoom = base::Lerp(std::log(2.0f), std::log(0.000014628f), linear);
            float zoom = exp(logZoom);
            TestConsts tempConsts;
            tempConsts.ZoomOffset = base::Vector2(-0.743643135f, -0.131825963f);
            tempConsts.ZoomScale = base::Vector2(zoom, zoom);
            tempConsts.SideCount = SIDE_RESOLUTION;
            tempConsts.MaxIterations = (uint32_t)(256 + 768 * linear);
            auto uploadedConsts = cmd.opUploadConstants(tempConsts);

            {
                TestParamsWrite tempParams;
                tempParams.Params = uploadedConsts;
                tempParams.Colors = m_imageWritable;
                cmd.opBindParametersInline("TestParamsWrite"_id, tempParams);

                cmd.opDispatch(m_shaderGenerate, SIDE_RESOLUTION / 8, SIDE_RESOLUTION / 8);
            }

            // we will wait for the buffer to be generated
            cmd.opGraphicsBarrier();

            // transition the data to reading format
            cmd.opImageLayoutBarrier(m_imageWritable, ImageLayout::ShaderReadOnly);

            {
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

                cmd.opBeingPass(fb);

                TestParamsRead tempParams;
                tempParams.Colors = m_imageWritable;
                cmd.opBindParametersInline("TestParamsRead"_id, tempParams);

                cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
                cmd.opDraw(m_shaderTest, 0, 6);
                cmd.opEndPass();
            }
        }

    } // test
} // rendering