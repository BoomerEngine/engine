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
        /// test of a dynamic imagea
        class RenderingTest_DynamicImageUpdate : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_DynamicImageUpdate, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

        private:
			static const uint32_t IMAGE_SIZE = 512;

			base::image::ImagePtr m_stage;

			ImageObjectPtr m_sampledImage;
			ImageSampledViewPtr m_sampledImageSRV;
            GraphicsPipelineObjectPtr m_shaders;
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

        void RenderingTest_DynamicImageUpdate::initialize()
        {
            ImageCreationInfo info;
            info.allowDynamicUpdate = true; // important for dynamic update
            info.allowShaderReads = true;
            info.width = IMAGE_SIZE;
            info.height = IMAGE_SIZE;
            info.format = ImageFormat::RGBA8_UNORM;
            m_sampledImage = createImage(info);
			m_sampledImageSRV = m_sampledImage->createSampledView();
            
            m_shaders = loadGraphicsShader("GenericGeometryWithTexture.csl", outputLayoutNoDepth());

            m_stage = base::RefNew<base::image::Image>(base::image::PixelFormat::Uint8_Norm, 4, IMAGE_SIZE, IMAGE_SIZE);
            base::image::Fill(m_stage->view(), &base::Vector4::ZERO());
        }

        struct Params
        {
            float x;
            float y;
            float radius;
            base::Vector4 color;
        };

        void RenderingTest_DynamicImageUpdate::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
        {
            // update
            {
                float halfX = IMAGE_SIZE / 2.0f;
                float halfY = IMAGE_SIZE / 2.0f;

                auto posX = halfX + halfX * cosf(time * TWOPI / 5.0f);
                auto posY = halfY + halfY * cosf(time * TWOPI / 6.0f);

                auto colorR = 0.7f + 0.3f * cosf(time / 10.0f);
                auto colorG = 0.7f - 0.3f * cosf(time / 11.0f);
                auto color = base::Color::FromVectorLinear(base::Vector4(colorR, colorG, 0.0f, 1.0f));

                static const int maxRadius = 30;
                float radius = 29.0f;

                auto minX = std::max<int>(0, std::floor(posX - maxRadius));
                auto minY = std::max<int>(0, std::floor(posY - maxRadius));
                auto maxX = std::min<int>(IMAGE_SIZE, std::ceil(posX + maxRadius));
                auto maxY = std::min<int>(IMAGE_SIZE, std::ceil(posY + maxRadius));

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
				cmd.opTransitionLayout(m_sampledImage, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);
                cmd.opUpdateDynamicImage(m_sampledImage, dirtyAreaView, 0, 0, minX, minY);
				cmd.opTransitionLayout(m_sampledImage, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
            }

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(outputLayoutNoDepth(), fb);

			DescriptorEntry desc[1];
			desc[0] = m_sampledImageSRV;
			cmd.opBindDescriptor("TestParams"_id, desc);

			drawQuad(cmd, m_shaders);

            cmd.opDraw(m_shaders, 0, 6); // quad

            cmd.opEndPass();
        }

    } // test
} // rendering
