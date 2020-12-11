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
        class RenderingTest_RenderToMultipleTextures : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_RenderToMultipleTextures, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

        private:
			static const uint32_t NUM_COLOR_BUFFERS = 4;

            SimpleScenePtr m_scene;
            SceneCamera m_camera;

            GraphicsPipelineObjectPtr m_shaderDraw;
			GraphicsPipelineObjectPtr m_shaderPreview;

            ImageObjectPtr m_colorBuffer[NUM_COLOR_BUFFERS];
			RenderTargetViewPtr m_colorBufferRTV[NUM_COLOR_BUFFERS];
			ImageSampledViewPtr m_colorBufferSRV[NUM_COLOR_BUFFERS];

			ImageObjectPtr m_depthBuffer;
			RenderTargetViewPtr m_depthBufferRTV;
			ImageSampledViewPtr m_depthBufferSRV;

			GraphicsPassLayoutObjectPtr m_renderToTextureLayout;

            static const uint32_t SIZE = 512;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_RenderToMultipleTextures);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2550);
        RTTI_END_TYPE();

        //---

        void RenderingTest_RenderToMultipleTextures::initialize()
        {
			GraphicsPassLayoutSetup setup;
			setup.depth.format = ImageFormat::D24S8;
			setup.color[0].format = ImageFormat::RGB10_A2_UNORM; // color
			setup.color[1].format = ImageFormat::R32F; // depth
			setup.color[2].format = ImageFormat::R11FG11FB10F; // normal
			setup.color[3].format = ImageFormat::RG16F; // normal

			m_renderToTextureLayout = createPassLayout(setup);

			GraphicsRenderStatesSetup render;
			render.depth(true);
			render.depthWrite(true);
			render.depthFunc(CompareOp::LessEqual);

            // load shaders
            m_shaderDraw = loadGraphicsShader("GenericSceneDeferredOutput.csl", m_renderToTextureLayout, &render);
            m_shaderPreview = loadGraphicsShader("RenderToTexturePreviewColor.csl", outputLayoutNoDepth());

            // load scene
            m_scene = CreateTeapotScene(*this);

            // create render targets
			for (uint32_t i = 0; i < NUM_COLOR_BUFFERS; ++i)
			{
				rendering::ImageCreationInfo colorBuffer;
				colorBuffer.allowRenderTarget = true;
				colorBuffer.allowShaderReads = true;
				colorBuffer.format = setup.color[i].format;
				colorBuffer.width = SIZE;
				colorBuffer.height = SIZE;
				m_colorBuffer[i] = createImage(colorBuffer);
				m_colorBufferSRV[i] = m_colorBuffer[i]->createSampledView();
				m_colorBufferRTV[i] = m_colorBuffer[i]->createRenderTargetView();
			}
            
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

        void RenderingTest_RenderToMultipleTextures::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
        {
			// rotate the teapot
			{
				auto& teapot = m_scene->m_objects[1];
				teapot.m_params.LocalToWorld = base::Matrix::BuildRotation(base::Angles(0.0f, -30.0f + time * 20.0f, 0.0f));
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
				fb.color[0].view(m_colorBufferRTV[0]).clear(base::Vector4(0.2f, 0.2f, 0.2f, 1.0f));
				fb.color[1].view(m_colorBufferRTV[1]).clear(base::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
				fb.color[2].view(m_colorBufferRTV[2]).clear(base::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
				fb.color[3].view(m_colorBufferRTV[3]).clear(base::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
				fb.depth.view(m_depthBufferRTV).clearDepth(1.0f).clearStencil(0.0f);

                cmd.opBeingPass(m_renderToTextureLayout, fb);
                m_scene->draw(cmd, m_shaderDraw, camera);
                cmd.opEndPass();
            }

			// transition resources
			cmd.opTransitionLayout(m_colorBuffer[0], ResourceLayout::RenderTarget, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_colorBuffer[1], ResourceLayout::RenderTarget, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_colorBuffer[2], ResourceLayout::RenderTarget, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_colorBuffer[3], ResourceLayout::RenderTarget, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_depthBuffer, ResourceLayout::DepthWrite, ResourceLayout::ShaderResource);

			// render preview
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
            cmd.opBeingPass(outputLayoutNoDepth(), fb);
            {
				DescriptorEntry desc[1];
				desc[0] = m_colorBufferSRV[0];
                cmd.opBindDescriptor("TestParams"_id, desc);
				drawQuad(cmd, m_shaderPreview, -1.0f, -1.0f, 1.0f, 1.0f);
            }
			{
				DescriptorEntry desc[1];
				desc[0] = m_colorBufferSRV[1];
				cmd.opBindDescriptor("TestParams"_id, desc);
				drawQuad(cmd, m_shaderPreview, 0.0f, -1.0f, 1.0f, 1.0f);
			}
			{
				DescriptorEntry desc[1];
				desc[0] = m_colorBufferSRV[2];
				cmd.opBindDescriptor("TestParams"_id, desc);
				drawQuad(cmd, m_shaderPreview, -1.0f, 0.0f, 1.0f, 1.0f);
			}
			{
				DescriptorEntry desc[1];
				desc[0] = m_colorBufferSRV[3];
				cmd.opBindDescriptor("TestParams"_id, desc);
				drawQuad(cmd, m_shaderPreview, 0.0f, 0.0f, 1.0f, 1.0f);
			}
            cmd.opEndPass();
        }

		//--

    } // test
} // rendering