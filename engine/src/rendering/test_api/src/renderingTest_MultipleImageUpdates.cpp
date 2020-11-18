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
#include "base/image/include/imageView.h"

namespace rendering
{
    namespace test
    {
        /// test of a multiple image updates within one frame
        class RenderingTest_MultipleImageUpdates : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_MultipleImageUpdates, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView) override final;

        private:
            static const uint32_t IMAGE_SIZE = 8;

            ImageView m_sampledImage;
            BufferView m_vertexBuffer;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_MultipleImageUpdates);
        RTTI_METADATA(RenderingTestOrderMetadata).order(2030);
        RTTI_END_TYPE();

        //---       

        static void PrepareTestGeometry(float x, float y, float w, float h, base::Array<Simple3DVertex>& outVertices)
        {
            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y, 0.5f, 1.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));

            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));
            outVertices.pushBack(Simple3DVertex(x, y + h, 0.5f, 0.0f, 1.0f));
        }

        namespace
        {
            struct TestConsts
            {
                base::Vector2 Offset;
                base::Vector2 Scale;
            };

            struct TestParams
            {
                ConstantsView Consts;
                ImageView TestTexture;
            };
        }

        void RenderingTest_MultipleImageUpdates::initialize()
        {
            m_shaders = loadShader("MultipleImageUpdates.csl");

            // generate test geometry
            {
                base::Array<Simple3DVertex> vertices;
                PrepareTestGeometry(0.1f, 0.1f, 0.8f, 0.8f, vertices);
                m_vertexBuffer = createVertexBuffer(vertices);
            }

            ImageCreationInfo info;
            info.allowShaderReads = true;
            info.allowDynamicUpdate = true;
            info.width = IMAGE_SIZE;
            info.height = IMAGE_SIZE;
            info.format = ImageFormat::RGBA8_UNORM;
            m_sampledImage = createImage(info);
        }

        void RenderingTest_MultipleImageUpdates::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView)
        {
            auto GRID_SIZE = 16;

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            // draw the grid
            base::FastRandState rng;
            for (uint32_t y = 0; y < GRID_SIZE; ++y)
            {
                for (uint32_t x = 0; x < GRID_SIZE; ++x)
                {
                    // setup pos
                    TestConsts consts;
                    consts.Scale.x = 2.0f / (1 + GRID_SIZE);
                    consts.Scale.y = 2.0f / (1 + GRID_SIZE);
                    consts.Offset.x = -1.0f + consts.Scale.x * 0.5f + (x * consts.Scale.x);
                    consts.Offset.y = -1.0f + consts.Scale.y * 0.5f + (y * consts.Scale.y);

                    // setup params
                    TestParams params;
                    params.Consts = cmd.opUploadConstants(consts);
                    params.TestTexture = m_sampledImage;
                    cmd.opBindParametersInline("TestParams"_id, params);

                    // update the content
                    {
                        // prepare data
                        base::Color updateColors[IMAGE_SIZE*IMAGE_SIZE];
                        base::Color randomColor = base::Color(rng.next(), rng.next(), rng.next());
                        for (uint32_t i=0; i<ARRAY_COUNT(updateColors); ++i)
                            updateColors[i] = randomColor;

                        // send update
                        base::image::ImageView updateView(base::image::NATIVE_LAYOUT, base::image::PixelFormat::Uint8_Norm, 4, &updateColors, IMAGE_SIZE, IMAGE_SIZE);
                        cmd.opUpdateDynamicImage(m_sampledImage, updateView);
                    }

                    // draw the quad
                    cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
                    cmd.opDraw(m_shaders, 0, 6); // quad
                }
            }

            // exit pass
            cmd.opEndPass();
        }

    } // test
} // rendering
