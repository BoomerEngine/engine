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

namespace rendering
{
    namespace test
    {
        /// test of the render to texture functionality
        class RenderingTest_RenderToTexture : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_RenderToTexture, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

			virtual void describeSubtest(base::IFormatStream& f) override
			{
				switch (subTestIndex())
				{
				case 0: f << "ColorTarget"; break;
				case 1: f << "DepthTarget"; break;
				case 2: f << "ColorTargetWithClear"; break;
				case 3: f << "DepthTargetWithClear"; break;
				}
			}

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

			base::FastRandState m_rnd;

            static const uint32_t SIZE = 512;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_RenderToTexture);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2500);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(4);
        RTTI_END_TYPE();

        //---

        void RenderingTest_RenderToTexture::initialize()
        {
			m_showDepth = subTestIndex() & 1;
			m_externalClear = subTestIndex() >= 2;

			GraphicsPassLayoutSetup setup;
			setup.depth.format = ImageFormat::D24S8;
			setup.depth.loadOp = m_externalClear ? LoadOp::Keep : LoadOp::Clear;
			setup.color[0].format = ImageFormat::RGBA8_UNORM;
			setup.color[0].loadOp = m_externalClear ? LoadOp::Keep : LoadOp::Clear;

			m_renderToTextureLayout = createPassLayout(setup);

			GraphicsRenderStatesSetup render;
			render.depth(true);
			render.depthWrite(true);
			render.depthFunc(CompareOp::LessEqual);

            // load shaders
            m_shaderDraw = loadGraphicsShader("GenericScene.csl", m_renderToTextureLayout, &render);

            if (m_showDepth)
                m_shaderPreview = loadGraphicsShader("RenderToTexturePreviewDepth.csl", outputLayoutNoDepth());
            else
                m_shaderPreview = loadGraphicsShader("RenderToTexturePreviewColor.csl", outputLayoutNoDepth());

            // load scene
            m_scene = CreateTeapotScene(*this);

            // create render targets
            rendering::ImageCreationInfo colorBuffer;
            colorBuffer.allowRenderTarget = true;
            colorBuffer.allowShaderReads = true;
            colorBuffer.format = rendering::ImageFormat::RGBA8_UNORM;
            colorBuffer.width = SIZE;
            colorBuffer.height = SIZE;
            m_colorBuffer = createImage(colorBuffer);
			m_colorBufferSRV = m_colorBuffer->createSampledView();
			m_colorBufferRTV = m_colorBuffer->createRenderTargetView();
            
            rendering::ImageCreationInfo depthBuffer;
            depthBuffer.allowRenderTarget = true;
            depthBuffer.allowShaderReads = true;
            depthBuffer.format = rendering::ImageFormat::D24S8;
            depthBuffer.width = SIZE;
            depthBuffer.height = SIZE;
            m_depthBuffer = createImage(depthBuffer);
			m_depthBufferSRV = m_depthBuffer->createSampledView();
			m_depthBufferRTV = m_depthBuffer->createRenderTargetView();
        }

        void RenderingTest_RenderToTexture::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
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
                camera.position = base::Vector3(-4.5f, 0.5f, 1.5f);
				camera.rotation = base::Angles(10.0f, 0.0f, 0.0f).toQuat();
				camera.calcMatrices();

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