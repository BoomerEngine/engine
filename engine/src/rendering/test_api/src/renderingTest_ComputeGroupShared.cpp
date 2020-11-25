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

        private:
            ShaderLibraryPtr m_shaderScene;
            ShaderLibraryPtr m_shaderLinearizeDepth;
            ShaderLibraryPtr m_shaderMinMaxDepth;
            ShaderLibraryPtr m_shaderNormalReconstruction;
            ShaderLibraryPtr m_shaderSSAO;
            ShaderLibraryPtr m_shaderPreview;

            uint8_t m_demo = 0;

            SimpleScenePtr m_scene;
            SceneCamera m_camera;

            ImageObjectPtr m_colorBuffer;
			ImageObjectPtr m_depthBuffer;

			RenderTargetViewPtr m_colorBufferRTV;
			ImageViewPtr m_colorBufferSRV;

			RenderTargetViewPtr m_depthBufferRTV;
			ImageViewPtr m_depthBufferSRV;

            ImageObjectPtr m_linearDepth;
			ImageWritableViewPtr m_linearDepthUAV;
			ImageViewPtr m_linearDepthSRV;

			ImageObjectPtr m_depth8x;
			ImageWritableViewPtr m_depth8xUAV;
			ImageViewPtr m_depth8xSRV;

			ImageObjectPtr m_normals2x;
			ImageWritableViewPtr m_normals2xUAV;
			ImageViewPtr m_normals2xSRV;

            static const uint32_t SIZE = 512;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ComputeGroupSharedMemory);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2230);
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
			m_colorBufferSRV = m_colorBuffer->createView(Globals().SamplerClampPoint);

            rendering::ImageCreationInfo depthBuffer;
            depthBuffer.allowShaderReads = true;
            depthBuffer.allowRenderTarget = true;
            depthBuffer.format = rendering::ImageFormat::D24S8;
            depthBuffer.width = SIZE;
            depthBuffer.height = SIZE;
            m_depthBuffer = createImage(depthBuffer);
			m_depthBufferRTV = m_depthBuffer->createRenderTargetView();
			m_depthBufferSRV = m_depthBuffer->createView(Globals().SamplerClampPoint);

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
				m_linearDepthSRV = m_linearDepth->createView(Globals().SamplerClampPoint);
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
				m_normals2xSRV = m_normals2x->createView(Globals().SamplerClampPoint);
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
				m_depth8xSRV = m_depth8x->createView(Globals().SamplerClampPoint);
            }

            m_shaderScene = loadShader("GenericScene.csl");
            m_shaderLinearizeDepth = loadShader("ComputeLinearizeDepth.csl");
            m_shaderMinMaxDepth = loadShader("ComputeMinMaxDepth.csl");
            m_shaderNormalReconstruction = loadShader("ComputeNormalReconstruction.csl");
            m_shaderSSAO = loadShader("ComputeSSAO.csl");
            m_shaderPreview = loadShader("GenericGeometryWithTexture.csl");
        }

        void RenderingTest_ComputeGroupSharedMemory::render(command::CommandWriter& cmd, float frameIndex, const RenderTargetView* backBufferView, const RenderTargetView* depth)
        {
            // setup scene camera
            SceneCamera camera;
            camera.cameraPosition = base::Vector3(-4.5f, 0.5f, 1.5f);
            camera.calcMatrices(true);

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
                cmd.opSetDepthState(true);
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
				desc[1] = m_depthBufferSRV;
				desc[2] = m_linearDepthUAV;
                cmd.opBindDescriptor("TestParams"_id, desc);

                cmd.opDispatch(m_shaderLinearizeDepth, SIZE / 8, SIZE / 8);
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

                cmd.opDispatch(m_shaderMinMaxDepth, SIZE / 8, SIZE / 8);

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

                cmd.opDispatch(m_shaderNormalReconstruction, SIZE / 8, SIZE / 8);

				cmd.opTransitionLayout(m_normals2x, ResourceLayout::UAV, ResourceLayout::ShaderResource);
            }
            else if (m_demo == 2)
            {
                struct Data
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

				DescriptorEntry desc[3];
				desc[0].constants(data);
				desc[1] = m_linearDepthSRV;
				desc[2] = m_normals2xUAV;
				cmd.opBindDescriptor("TestParams"_id, desc);

                cmd.opDispatch(m_shaderSSAO, SIZE / 8, SIZE / 8);

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
					desc[0] = m_depth8xSRV;
                else if (m_demo == 1)
					desc[0] = m_normals2xSRV;
                else if (m_demo == 2)
					desc[0] = m_normals2xSRV;
                cmd.opBindDescriptor("TestParams"_id, desc);

                drawQuad(cmd, m_shaderPreview, -1.0f, -1.0f, 2.0f, 2.0f);
                cmd.opEndPass();
            }
        }

    } // test
} // rendering