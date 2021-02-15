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
        class RenderingTest_ViewportIndex : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ViewportIndex, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

        private:
            SimpleScenePtr m_scene;
            SceneCamera m_camera;

            GraphicsPipelineObjectPtr m_shaderDraw;
			GraphicsPipelineObjectPtr m_shaderPreview;

            ImageObjectPtr m_colorBuffer;
			RenderTargetViewPtr m_colorBufferRTV;
			ImageSampledViewPtr m_colorBufferSRV;

			ImageObjectPtr m_depthBuffer;
			RenderTargetViewPtr m_depthBufferRTV;
			ImageSampledViewPtr m_depthBufferSRV;

			uint32_t m_numViewportsPerSide = 1;
			uint32_t m_numViewports = 1;

            static const uint32_t SIZE = 1024;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ViewportIndex);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2552);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(4);
        RTTI_END_TYPE();

        //---

        void RenderingTest_ViewportIndex::initialize()
        {
			m_numViewportsPerSide = 1 + subTestIndex();
			m_numViewports = m_numViewportsPerSide * m_numViewportsPerSide;

			GraphicsRenderStatesSetup render;
			render.depth(true);
			render.depthWrite(true);
			render.depthFunc(CompareOp::LessEqual);

            // load shaders
            m_shaderDraw = loadGraphicsShader("GenericSceneViewportIndexGS.csl", &render);
            m_shaderPreview = loadGraphicsShader("RenderToTexturePreviewColor.csl");

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

        void RenderingTest_ViewportIndex::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
        {
			// rotate the teapot
			{
				auto& teapot = m_scene->m_objects[1];
				teapot.m_params.LocalToWorld = base::Matrix::BuildRotation(base::Angles(0.0f, -30.0f + time * 20.0f, 0.0f));
			}
			
			// pass viewport count to shaders
			{
				DescriptorEntry desc[1];
				desc[0].constants(m_numViewports);
				cmd.opBindDescriptor("TestParams"_id, desc);
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
				fb.color[0].view(m_colorBufferRTV).clear(base::Vector4(0.2f, 0.2f, 0.2f, 1.0f));
				fb.depth.view(m_depthBufferRTV).clearDepth(1.0f).clearStencil(0.0f);

                cmd.opBeingPass(fb, m_numViewports);

				{
					const auto viewportSize = SIZE / m_numViewportsPerSide;
					uint32_t viewportIndex = 0;
					for (uint32_t y = 0; y < m_numViewportsPerSide; ++y)
					{
						for (uint32_t x = 0; x < m_numViewportsPerSide; ++x)
						{
							cmd.opSetViewportRect(viewportIndex, x * viewportSize, y * viewportSize, viewportSize, viewportSize);
							viewportIndex += 1;
						}
					}
				}

                m_scene->draw(cmd, m_shaderDraw, camera);

                cmd.opEndPass();
            }

			// transition resources
			cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::RenderTarget, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_depthBuffer, ResourceLayout::DepthWrite, ResourceLayout::ShaderResource);
			
			// render preview
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
            cmd.opBeingPass(fb);
            {
				DescriptorEntry desc[1];
				desc[0] = m_colorBufferSRV;
                cmd.opBindDescriptor("TestParams"_id, desc);
                drawQuad(cmd, m_shaderPreview);
            }
            cmd.opEndPass();
        }

		//--

    } // test
} // rendering