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
        /// test of the compute group shared memory
        class RenderingTest_ComputeGroupSharedMemory : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ComputeGroupSharedMemory, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float frameIndex, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

			virtual void describeSubtest(base::IFormatStream& f) override
			{
				switch (subTestIndex())
				{
					case 0: f << "LinearizeDepth"; break;
					case 1: f << "ReconstructNormals"; break;
					case 2: f << "SimpleSSAO"; break;
				}
			}

        private:
            GraphicsPipelineObjectPtr m_shaderScene;
            ComputePipelineObjectPtr m_shaderLinearizeDepth;
			ComputePipelineObjectPtr m_shaderMinMaxDepth;
			ComputePipelineObjectPtr m_shaderNormalReconstruction;
			ComputePipelineObjectPtr m_shaderSSAO;
            GraphicsPipelineObjectPtr m_shaderPreview;

            uint8_t m_demo = 0;

            SimpleScenePtr m_scene;
            SceneCamera m_camera;

            ImageObjectPtr m_colorBuffer;
			ImageObjectPtr m_depthBuffer;

			RenderTargetViewPtr m_colorBufferRTV;
			ImageReadOnlyViewPtr m_colorBufferSRV;

			RenderTargetViewPtr m_depthBufferRTV;
			ImageSampledViewPtr m_depthBufferSRVSampled;

            ImageObjectPtr m_linearDepth;
			ImageWritableViewPtr m_linearDepthUAV;
			ImageReadOnlyViewPtr m_linearDepthSRV;

			ImageObjectPtr m_depth8x;
			ImageWritableViewPtr m_depth8xUAV;
			ImageReadOnlyViewPtr m_depth8xSRV;
			ImageSampledViewPtr m_depth8xSRVSampled;

			ImageObjectPtr m_normals2x;
			ImageWritableViewPtr m_normals2xUAV;
			ImageReadOnlyViewPtr m_normals2xSRV;
			ImageSampledViewPtr m_normals2xSRVSampled;

            static const uint32_t SIZE = 512;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ComputeGroupSharedMemory);
            RTTI_METADATA(RenderingTestOrderMetadata).order(4600);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(3);
        RTTI_END_TYPE();

        //---


        void RenderingTest_ComputeGroupSharedMemory::initialize()
        {
            // the demo
            m_demo = subTestIndex();

            // load scene
            m_scene = CreateTeapotScene(*this);

            // create render targets
            rendering::ImageCreationInfo colorBuffer;
            colorBuffer.allowShaderReads = true;
            colorBuffer.allowRenderTarget = true;
            colorBuffer.format = rendering::ImageFormat::RGBA8_UNORM;
            colorBuffer.width = SIZE;
            colorBuffer.height = SIZE;
            m_colorBuffer = createImage(colorBuffer);
			m_colorBufferRTV = m_colorBuffer->createRenderTargetView();
			m_colorBufferSRV = m_colorBuffer->createReadOnlyView();

            rendering::ImageCreationInfo depthBuffer;
            depthBuffer.allowShaderReads = true;
            depthBuffer.allowRenderTarget = true;
            depthBuffer.format = rendering::ImageFormat::D24S8;
            depthBuffer.width = SIZE;
            depthBuffer.height = SIZE;
            m_depthBuffer = createImage(depthBuffer);
			m_depthBufferRTV = m_depthBuffer->createRenderTargetView();
			m_depthBufferSRVSampled = m_depthBuffer->createSampledView();

            // create linear depth buffer
            {
                rendering::ImageCreationInfo tempBuffer;
                tempBuffer.allowShaderReads = true;
                tempBuffer.allowUAV = true;
                tempBuffer.format = rendering::ImageFormat::R32F;
                tempBuffer.width = SIZE;
                tempBuffer.height = SIZE;
                m_linearDepth = createImage(tempBuffer);
				m_linearDepthUAV = m_linearDepth->createWritableView();
				m_linearDepthSRV = m_linearDepth->createReadOnlyView();
            }

            // normal reconstruction buffer
            {
                rendering::ImageCreationInfo tempBuffer;
                tempBuffer.allowShaderReads = true;
                tempBuffer.allowUAV = true;
                tempBuffer.format = rendering::ImageFormat::RGBA8_UNORM;
                tempBuffer.width = SIZE;// / 2;
                tempBuffer.height = SIZE;// / 2;
                m_normals2x = createImage(tempBuffer);
				m_normals2xUAV = m_normals2x->createWritableView();
				m_normals2xSRV = m_normals2x->createReadOnlyView();
				m_normals2xSRVSampled = m_normals2x->createSampledView();
            }

            // create downsampled buffer
            {
                rendering::ImageCreationInfo tempBuffer;
                tempBuffer.allowShaderReads = true;
                tempBuffer.allowUAV = true;
                tempBuffer.format = rendering::ImageFormat::RG16F;
                tempBuffer.width = SIZE / 8;
                tempBuffer.height = SIZE / 8;
                m_depth8x = createImage(tempBuffer);
				m_depth8xUAV = m_depth8x->createWritableView();
				m_depth8xSRV = m_depth8x->createReadOnlyView();
				m_depth8xSRVSampled = m_depth8x->createSampledView();
            }

			{
				GraphicsRenderStatesSetup render;
				render.depth(true);
				render.depthWrite(true);
				render.depthFunc(CompareOp::LessEqual);

				m_shaderScene = loadGraphicsShader("GenericScene.csl", &render);
			}

            m_shaderLinearizeDepth = loadComputeShader("ComputeLinearizeDepth.csl");
            m_shaderMinMaxDepth = loadComputeShader("ComputeMinMaxDepth.csl");
            m_shaderNormalReconstruction = loadComputeShader("ComputeNormalReconstruction.csl");
            m_shaderSSAO = loadComputeShader("ComputeSSAO.csl");
            m_shaderPreview = loadGraphicsShader("GenericGeometryWithTexture.csl");
        }

        void RenderingTest_ComputeGroupSharedMemory::render(command::CommandWriter& cmd, float frameIndex, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
            // setup scene camera
            SceneCamera camera;
			camera.position = base::Vector3(-4.5f, 0.5f, 1.5f);
			camera.rotation = base::Angles(10.0f, 0.0f, 0.0f).toQuat();
			camera.calcMatrices();

            //-----------
            // STEP 0. Rotate teapot

            {
                auto& teapot = m_scene->m_objects[1];
                teapot.m_params.LocalToWorld = base::Matrix::BuildRotation(base::Angles(0.0f, -30.0f + frameIndex * 20.0f, 0.0f));
            }

            //-----------
            // STEP 1. Render scene
            {
                // render shit to render targets
                FrameBuffer fb;
                fb.color[0].view(m_colorBufferRTV).clear(0.2f, 0.2f, 0.2f, 1.0f);
                fb.depth.view(m_depthBufferRTV).clearDepth().clearStencil();

                cmd.opBeingPass(fb);
                m_scene->draw(cmd, m_shaderScene, camera);
                cmd.opEndPass();
            }

            // transition the data to format of the next pass
            cmd.opTransitionLayout(m_depthBuffer, ResourceLayout::DepthWrite, ResourceLayout::ShaderResource);
			cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::RenderTarget, ResourceLayout::ShaderResource);

            //-----------
            // STEP 2. Linearize depth

            {
                base::Vector4 data(0,0,0,0);
                data.z = camera.m_ViewToScreen.m[2][2];
                data.w = camera.m_ViewToScreen.m[2][3];

				DescriptorEntry desc[3];
				desc[0].constants(data);
				desc[1] = m_depthBufferSRVSampled;
				desc[2] = m_linearDepthUAV;
                cmd.opBindDescriptor("TestParams"_id, desc);

                cmd.opDispatchThreads(m_shaderLinearizeDepth, SIZE, SIZE);
            }

			// transition the data to format of the next pass
			cmd.opTransitionLayout(m_linearDepth, ResourceLayout::UAV, ResourceLayout::ShaderResource);

            //-----------
            // STEP 3. Demo

            if (m_demo == 0)
            {
                base::Vector4 data(0,0,0,0);

				DescriptorEntry desc[3];
				desc[0].constants(data);
				desc[1] = m_linearDepthSRV;
				desc[2] = m_depth8xUAV;
                cmd.opBindDescriptor("TestParams"_id, desc);

                cmd.opDispatchThreads(m_shaderMinMaxDepth, SIZE, SIZE);

				cmd.opTransitionLayout(m_depth8x, ResourceLayout::UAV, ResourceLayout::ShaderResource);
            }
            else if (m_demo == 1)
            {
                struct
                {
                    base::Vector4 Params;
                    base::Matrix PixelCoordToWorld;
                } data;

                base::Matrix pixelCoordToScreen;
                pixelCoordToScreen.identity();
                pixelCoordToScreen.m[0][0] = 2.0f / SIZE;
                pixelCoordToScreen.m[1][1] = 2.0f / SIZE;
                pixelCoordToScreen.m[0][3] = -1.0f;
                pixelCoordToScreen.m[1][3] = -1.0f;

                data.Params = base::Vector4::ZERO();
                data.PixelCoordToWorld = pixelCoordToScreen * camera.m_ScreenToWorld;

				DescriptorEntry desc[3];
				desc[0].constants(data);
				desc[1] = m_linearDepthSRV;
				desc[2] = m_normals2xUAV;
				cmd.opBindDescriptor("TestParams"_id, desc);

                cmd.opDispatchThreads(m_shaderNormalReconstruction, SIZE, SIZE);

				cmd.opTransitionLayout(m_normals2x, ResourceLayout::UAV, ResourceLayout::ShaderResource);
            }
            else if (m_demo == 2)
            {
                struct Data
                {
                    base::Matrix PixelCoordToWorld;
                } data;

                data.PixelCoordToWorld.identity();
                data.PixelCoordToWorld.m[0][0] = 2.0f / SIZE;
                data.PixelCoordToWorld.m[1][1] = 2.0f / SIZE;
                data.PixelCoordToWorld.m[0][3] = -1.0f;
                data.PixelCoordToWorld.m[1][3] = -1.0f;

				DescriptorEntry desc[3];
				desc[0].constants(data);
				desc[1] = m_linearDepthSRV;
				desc[2] = m_normals2xUAV;
				cmd.opBindDescriptor("TestParams"_id, desc);

                cmd.opDispatchThreads(m_shaderSSAO, SIZE, SIZE);

				cmd.opTransitionLayout(m_normals2x, ResourceLayout::UAV, ResourceLayout::ShaderResource);
            }

            //-----------
            // STEP 4. Render preview
            {
                // render the preview quad
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(0.0f, 0.0f, 0.2f, 1.0f);
                cmd.opBeingPass(fb);

				DescriptorEntry desc[1];
				if (m_demo == 0)
					desc[0] = m_depth8xSRVSampled;
                else if (m_demo == 1)
					desc[0] = m_normals2xSRVSampled;
                else if (m_demo == 2)
					desc[0] = m_normals2xSRVSampled;
                cmd.opBindDescriptor("TestParams"_id, desc);

                drawQuad(cmd, m_shaderPreview, -1.0f, -1.0f, 2.0f, 2.0f);
                cmd.opEndPass();
            }
        }

    } // test
} // rendering