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
#include "base/image/include/imageUtils.h"
#include "base/image/include/imageView.h"
#include "base/image/include/image.h"

namespace rendering
{
    namespace test
    {
		
		class TestDownloadSink : public IDownloadDataSink
		{
		public:
			TestDownloadSink(uint32_t x, uint32_t y)
				: m_x(x), m_y(y)
			{}

			const uint32_t m_x;
			const uint32_t m_y;

			base::image::ImagePtr retrieveImage()
			{
				auto lock = base::CreateLock(m_lock);
				auto ret = std::move(m_image);
				return ret;
			}

			void processRetreivedData(const void* srcPtr, uint32_t retrievedSize, const ResourceCopyRange& info) override final
			{
				base::image::ImageView srcView(base::image::NATIVE_LAYOUT, base::image::PixelFormat::Uint8_Norm, 4, srcPtr, info.image.sizeX, info.image.sizeY);
				ASSERT(srcView.dataSize() == retrievedSize);

				auto newImage = base::RefNew<base::image::Image>(srcView);

				auto lock = base::CreateLock(m_lock);
				m_image = newImage;
			}

		private:
			base::SpinLock m_lock;
			base::image::ImagePtr m_image;
		};


        /// test of the render to texture functionality
        class RenderingTest_DownloadImage : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_DownloadImage, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

        private:
            SimpleScenePtr m_scene;
            SceneCamera m_camera;
            bool m_showDepth = false;
			bool m_externalClear = false;
			bool m_firstFrame = true;

            GraphicsPipelineObjectPtr m_shaderDraw;
			GraphicsPipelineObjectPtr m_shaderPreview;

            ImageObjectPtr m_colorBuffer;
			RenderTargetViewPtr m_colorBufferRTV;
			ImageSampledViewPtr m_colorBufferSRV;

			ImageObjectPtr m_depthBuffer;
			RenderTargetViewPtr m_depthBufferRTV;
			ImageSampledViewPtr m_depthBufferSRV;

			GraphicsPassLayoutObjectPtr m_renderToTextureLayout;

			ImageObjectPtr m_displayImage;
			ImageSampledViewPtr m_displayImageSRV;

			base::RefPtr<TestDownloadSink> m_download;

			base::FastRandState m_rnd;

			base::image::ImagePtr m_stage;

            static const uint32_t SIZE = 512;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_DownloadImage);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2510);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(2);
        RTTI_END_TYPE();

        //---

        void RenderingTest_DownloadImage::initialize()
        {
			m_showDepth = subTestIndex() & 1;

			GraphicsPassLayoutSetup setup;
			setup.depth.format = ImageFormat::D24S8;
			setup.color[0].format = ImageFormat::RGBA8_UNORM;

			m_renderToTextureLayout = createPassLayout(setup);

			GraphicsRenderStatesSetup render;
			render.depth(true);
			render.depthWrite(true);
			render.depthFunc(CompareOp::LessEqual);

            // load shaders
            m_shaderDraw = loadGraphicsShader("GenericScene.csl", m_renderToTextureLayout, &render);
			m_shaderPreview = loadGraphicsShader("GenericGeometryWithTexture.csl", outputLayoutNoDepth());

            // load scene
            m_scene = CreateTeapotScene(*this);

            // create render targets
            rendering::ImageCreationInfo colorBuffer;
            colorBuffer.allowRenderTarget = true;
            colorBuffer.allowShaderReads = true;
			colorBuffer.allowCopies = true;
            colorBuffer.format = rendering::ImageFormat::RGBA8_UNORM;
            colorBuffer.width = SIZE;
            colorBuffer.height = SIZE;
            m_colorBuffer = createImage(colorBuffer);
			m_colorBufferSRV = m_colorBuffer->createSampledView();
			m_colorBufferRTV = m_colorBuffer->createRenderTargetView();
            
            rendering::ImageCreationInfo depthBuffer;
            depthBuffer.allowRenderTarget = true;
            depthBuffer.allowShaderReads = true;
			depthBuffer.allowCopies = true;
            depthBuffer.format = rendering::ImageFormat::D24S8;
            depthBuffer.width = SIZE;
            depthBuffer.height = SIZE;
            m_depthBuffer = createImage(depthBuffer);
			m_depthBufferSRV = m_depthBuffer->createSampledView();
			m_depthBufferRTV = m_depthBuffer->createRenderTargetView();

			ImageCreationInfo info;
			info.allowDynamicUpdate = true; // important for dynamic update
			info.allowShaderReads = true;
			info.width = SIZE;
			info.height = SIZE;
			info.format = ImageFormat::RGBA8_UNORM;
			m_displayImage = createImage(info);
			m_displayImageSRV = m_displayImage->createSampledView();

			m_stage = base::RefNew<base::image::Image>(base::image::PixelFormat::Uint8_Norm, 4, SIZE, SIZE);
			base::image::Fill(m_stage->view(), &base::Vector4::ZERO());
        }

		void RenderingTest_DownloadImage::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
		{
			// rotate the teapot
			{
				auto& teapot = m_scene->m_objects[1];
				teapot.m_params.LocalToWorld = base::Matrix::BuildRotation(base::Angles(0.0f, -30.0f + time * 20.0f, 0.0f));
			}

			// clear render targets
			if (m_externalClear)
			{
				int frameIndex = (int)(time * 100.0f);

				auto w = 16 + m_rnd.range(SIZE / 4);
				auto h = 16 + m_rnd.range(SIZE / 4);
				auto x = m_rnd.range(SIZE - w - 1);
				auto y = m_rnd.range(SIZE - h - 1);

				base::Rect rect;
				rect.min.x = x;
				rect.min.y = y;
				rect.max.x = x + w;
				rect.max.y = y + h;

				if (frameIndex % 60 == 0)
				{
					float clearDepth = m_rnd.range(0.996f, 1.0f);
					cmd.opClearDepthStencil(m_depthBufferRTV, true, true, clearDepth, 0, &rect, 1);
				}
				else if (frameIndex % 10 == 0)
				{
					base::Vector4 color;
					color.x = m_rnd.unit();
					color.y = m_rnd.unit();
					color.z = m_rnd.unit();
					color.w = 1.0f;

					cmd.opClearRenderTarget(m_colorBufferRTV, color, &rect, 1);
				}
			}

			// render scene
			{
				// setup scene camera
				SceneCamera camera;
				camera.cameraPosition = base::Vector3(-4.5f, 0.5f, 1.5f);
				camera.calcMatrices(true);

				// render shit to render targets
				FrameBuffer fb;
				if (m_externalClear && !m_firstFrame)
				{
					fb.color[0].view(m_colorBufferRTV);
					fb.depth.view(m_depthBufferRTV);
				}
				else
				{
					fb.color[0].view(m_colorBufferRTV).clear(base::Vector4(0.2f, 0.2f, 0.2f, 1.0f));
					fb.depth.view(m_depthBufferRTV).clearDepth(1.0f).clearStencil(0.0f);
				}

				cmd.opBeingPass(m_renderToTextureLayout, fb);
				m_scene->draw(cmd, m_shaderDraw, camera);
				cmd.opEndPass();
			}

			// transition resources
			cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::RenderTarget, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_depthBuffer, ResourceLayout::DepthWrite, ResourceLayout::ShaderResource);

			// get copy ?
			if (m_download)
			{
				if (auto image = m_download->retrieveImage())
				{
					// copy image into stage
					auto targetView = m_stage->view().subView(m_download->m_x, m_download->m_y, image->width(), image->height());
					base::image::Copy(image->view(), targetView);

					// update preview
					{
						cmd.opTransitionLayout(m_displayImage, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);
						cmd.opUpdateDynamicImage(m_displayImage, targetView, 0, 0, m_download->m_x, m_download->m_y);
						cmd.opTransitionLayout(m_displayImage, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
					}

					m_download.reset();
				}
			}

			// start new copy ?
			if (!m_download)
			{
				auto w = 16 + m_rnd.range(64);
				auto h = 16 + m_rnd.range(64);
				auto sx = m_rnd.range(m_colorBufferRTV->width() - w);
				auto sy = m_rnd.range(m_colorBufferRTV->height() - h);
				auto dx = sx;// rnd.range(m_colorBufferRTV->width() - w);
				auto dy = sy;// rnd.range(m_colorBufferRTV->height() - h);

				m_download = base::RefNew<TestDownloadSink>(dx, dy);

				ResourceCopyRange srcRange;
				srcRange.image.mip = 0;
				srcRange.image.slice = 0;
				srcRange.image.offsetX = sx;
				srcRange.image.offsetY = sy;
				srcRange.image.offsetZ = 0;
				srcRange.image.sizeX = w;
				srcRange.image.sizeY = h;
				srcRange.image.sizeZ = 1;

				cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::ShaderResource, ResourceLayout::CopySource);
				cmd.opDownloadData(m_colorBuffer, srcRange, m_download);

				cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::CopySource, ResourceLayout::ShaderResource);
			}

			// render preview
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
            cmd.opBeingPass(outputLayoutNoDepth(), fb);
            {
				DescriptorEntry desc[1];
				desc[0] = m_showDepth ? m_depthBufferSRV : m_colorBufferSRV;
                cmd.opBindDescriptor("TestParams"_id, desc);

                drawQuad(cmd, m_shaderPreview, -0.8f, -0.8f, 1.6f, 1.6f);
            }
            cmd.opEndPass();

			m_firstFrame = false;
        }

    } // test
} // rendering