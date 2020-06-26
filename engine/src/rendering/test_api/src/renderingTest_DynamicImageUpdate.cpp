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
#include "base/image/include/imageView.h"

namespace rendering
{
    namespace test
    {
        /// test of a dynamic imagea
        class RenderingTest_DynamicImageUpdate : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_DynamicImageUpdate, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView) override final;

        private:
            ImageView m_sampledImage;
            BufferView m_vertexBuffer;
            base::image::ImagePtr m_stage;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_DynamicImageUpdate);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2000);
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
            struct TestParams
            {
                ImageView TestTexture;
            };
        }

        void RenderingTest_DynamicImageUpdate::initialize()
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

            ImageCreationInfo info;
            info.allowDynamicUpdate = true;
            info.allowShaderReads = true;
            info.width = 512;
            info.height = 512;
            info.format = ImageFormat::RGBA8_UNORM;
            m_sampledImage = createImage(info);
            
            m_shaders = loadShader("GenericGeometryWithTexture.csl");

            m_stage = base::CreateSharedPtr<base::image::Image>(base::image::PixelFormat::Uint8_Norm, 4, 512, 512);
            base::image::Fill(m_stage->view(), &base::Vector4::ZERO());
        }

        struct Params
        {
            float x;
            float y;
            float radius;
            base::Vector4 color;
        };

        void RenderingTest_DynamicImageUpdate::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView)
        {
            // update whole image on the first frame
            /*if (frameIndex == 0)
            {
                cmd.opUpdateDynamicImage(m_sampledImage, m_stage->view());
            }*/

            // update
            {
                uint32_t width = m_sampledImage.width();
                uint32_t height = m_sampledImage.height();
                float halfX = width / 2.0f;
                float halfY = height / 2.0f;

                auto posX = halfX + halfX * cosf(time * TWOPI / 5.0f);
                auto posY = halfY + halfY * cosf(time * TWOPI / 6.0f);

                auto colorR = 0.7f + 0.3f * cosf(time / 10.0f);
                auto colorG = 0.7f - 0.3f * cosf(time / 11.0f);
                auto color = base::Color::FromVectorLinear(base::Vector4(colorR, colorG, 0.0f, 1.0f));

                static const int maxRadius = 30;
                float radius = 29.0f;

                auto minX = std::max<int>(0, std::floor(posX - maxRadius));
                auto minY = std::max<int>(0, std::floor(posY - maxRadius));
                auto maxX = std::min<int>(width, std::ceil(posX + maxRadius));
                auto maxY = std::min<int>(height, std::ceil(posY + maxRadius));

                for (base::image::ImageViewRowIterator iy(m_stage->view(), minY, maxY); iy; ++iy)
                {
                    for (base::image::ImageViewPixelIteratorT<base::Color> ix(iy, minX, maxX); ix; ++ix)
                    {
                        float px = (float)ix.pos();
                        float py = (float)iy.pos();

                        float d = base::Vector2(px - posX, py - posY).length();
                        if (d <= radius)
                        {
                            auto frac = 1.0f - 0.5f * (d / radius);
                            ix.data() = color * frac;
                        }
                    }
                }
                
                auto dirtyAreaView = m_stage->view().subView(minX, minY, maxX - minX, maxY - minY);
                cmd.opUpdateDynamicImage(m_sampledImage, dirtyAreaView, minX, minY);
            }

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            TestParams tempParams;
            tempParams.TestTexture = m_sampledImage;
            cmd.opBindParametersInline("TestParams"_id,tempParams);

            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);

            cmd.opDraw(m_shaders, 0, 6); // quad

            cmd.opEndPass();
        }

    } // test
} // rendering
