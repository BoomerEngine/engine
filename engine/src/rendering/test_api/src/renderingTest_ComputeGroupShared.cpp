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
#include "renderingTestScene.h"

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingCommandWriter.h"

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
            virtual void render(command::CommandWriter& cmd, float frameIndex, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            const ShaderLibrary* m_shaderScene;
            const ShaderLibrary* m_shaderLinearizeDepth;
            const ShaderLibrary* m_shaderMinMaxDepth;
            const ShaderLibrary* m_shaderNormalReconstruction;
            const ShaderLibrary* m_shaderSSAO;
            const ShaderLibrary* m_shaderPreview;

            uint8_t m_demo = 0;

            SimpleScenePtr m_scene;
            SceneCamera m_camera;

            ImageView m_colorBuffer;
            ImageView m_depthBuffer;

            ImageView m_linearDepth;
            ImageView m_depth8x;
            ImageView m_normals2x;

            static const uint32_t SIZE = 512;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_ComputeGroupSharedMemory);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2230);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(3);
        RTTI_END_TYPE();

        //---

        namespace
        {
            struct TestParams
            {
                ConstantsView Constants;
                ImageView InputImage;
                ImageView OutputImage;
            };
        }

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

            rendering::ImageCreationInfo depthBuffer;
            depthBuffer.allowShaderReads = true;
            depthBuffer.allowRenderTarget = true;
            depthBuffer.format = rendering::ImageFormat::D24S8;
            depthBuffer.width = SIZE;
            depthBuffer.height = SIZE;
            m_depthBuffer = createImage(depthBuffer);

            // create linear depth buffer
            {
                rendering::ImageCreationInfo tempBuffer;
                tempBuffer.allowShaderReads = true;
                tempBuffer.allowUAV = true;
                tempBuffer.format = rendering::ImageFormat::R32F;
                tempBuffer.width = SIZE;
                tempBuffer.height = SIZE;
                m_linearDepth = createImage(tempBuffer);
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
            }

            m_shaderScene = loadShader("GenericScene.csl");
            m_shaderLinearizeDepth = loadShader("ComputeLinearizeDepth.csl");
            m_shaderMinMaxDepth = loadShader("ComputeMinMaxDepth.csl");
            m_shaderNormalReconstruction = loadShader("NormalReconstruction.csl");
            m_shaderSSAO = loadShader("ComputeSSAO.csl");
            m_shaderPreview = loadShader("GenericGeometryWithTexture.csl");
        }

        void RenderingTest_ComputeGroupSharedMemory::render(command::CommandWriter& cmd, float frameIndex, const ImageView& backBufferView, const ImageView& depth)
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
                fb.color[0].view(m_colorBuffer).clear(0.2f, 0.2f, 0.2f, 1.0f);
                fb.depth.view(m_depthBuffer).clearDepth().clearStencil();

                cmd.opBeingPass(fb);
                cmd.opSetDepthState(true);
                m_scene->draw(cmd, m_shaderScene, camera);
                cmd.opEndPass();
            }

            // we will wait for the buffer to be generated
            cmd.opGraphicsBarrier();

            // transition the data to reading format
            cmd.opImageLayoutBarrier(m_depthBuffer, ImageLayout::ShaderReadOnly);
            cmd.opImageLayoutBarrier(m_colorBuffer, ImageLayout::ShaderReadOnly);

            // get resolved buffers
            auto resolvedDepth = m_depthBuffer.createSampledView(ObjectID::DefaultPointSampler());

            //-----------
            // STEP 2. Linearize depth

            {
                base::Vector4 data(0,0,0,0);
                data.z = camera.m_ViewToScreen.m[2][2];
                data.w = camera.m_ViewToScreen.m[2][3];

                TestParams params;
                params.Constants = cmd.opUploadConstants(data);
                params.InputImage = resolvedDepth;
                params.OutputImage = m_linearDepth;
                cmd.opBindParametersInline("TestParams"_id, params);

                cmd.opDispatch(m_shaderLinearizeDepth, SIZE / 8, SIZE / 8);
            }

            // we will wait for the buffer to be generated
            cmd.opGraphicsBarrier();

            //-----------
            // STEP 3. Demo

            if (m_demo == 0)
            {
                base::Vector4 data(0,0,0,0);

                TestParams params;
                params.Constants = cmd.opUploadConstants(data);
                params.InputImage = m_linearDepth;
                params.OutputImage = m_depth8x;
                cmd.opBindParametersInline("TestParams"_id, params);

                cmd.opDispatch(m_shaderMinMaxDepth, SIZE / 8, SIZE / 8);
            }
            else if (m_demo == 1)
            {
                base::Vector4 data(0,0,0,0);

                struct Data
                {
                    base::Vector4 Params;
                    base::Matrix PixelCoordToWorld;
                };

                base::Matrix pixelCoordToScreen;
                pixelCoordToScreen.identity();
                pixelCoordToScreen.m[0][0] = 2.0f / SIZE;
                pixelCoordToScreen.m[1][1] = 2.0f / SIZE;
                pixelCoordToScreen.m[0][3] = -1.0f;
                pixelCoordToScreen.m[1][3] = -1.0f;

                Data dataX;
                dataX.Params = base::Vector4::ZERO();
                dataX.PixelCoordToWorld = pixelCoordToScreen * camera.m_ScreenToWorld;

                TestParams params;
                params.Constants = cmd.opUploadConstants(dataX);
                params.InputImage = resolvedDepth;//m_linearDepth.createReadOnlyView();
                params.OutputImage = m_normals2x;
                cmd.opBindParametersInline("TestParams"_id, params);

                cmd.opDispatch(m_shaderNormalReconstruction, SIZE / 8, SIZE / 8);
            }
            else if (m_demo == 2)
            {
                base::Vector4 data(0,0,0,0);

                struct Data
                {
                    base::Vector4 Params;
                    base::Matrix PixelCoordToWorld;
                };

                base::Matrix pixelCoordToScreen;
                pixelCoordToScreen.identity();
                pixelCoordToScreen.m[0][0] = 2.0f / SIZE;
                pixelCoordToScreen.m[1][1] = 2.0f / SIZE;
                pixelCoordToScreen.m[0][3] = -1.0f;
                pixelCoordToScreen.m[1][3] = -1.0f;

                Data dataX;
                dataX.Params = base::Vector4::ZERO();
                dataX.PixelCoordToWorld = pixelCoordToScreen * camera.m_ScreenToView;

                TestParams params;
                params.Constants = cmd.opUploadConstants(dataX);
                params.InputImage = resolvedDepth;//m_linearDepth.createReadOnlyView();
                params.OutputImage = m_normals2x;
                cmd.opBindParametersInline("TestParams"_id, params);

                cmd.opDispatch(m_shaderSSAO, SIZE / 8, SIZE / 8);
            }

            //-----------
            // STEP 4. Render preview
            {
                // render the preview quad
                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(0.0f, 0.0f, 0.2f, 1.0f);
                cmd.opBeingPass(fb);

                ImageView params;
                if (m_demo == 0)
                    params = m_depth8x.createSampledView(ObjectID::DefaultPointSampler());
                else if (m_demo == 1)
                    params = m_normals2x.createSampledView(ObjectID::DefaultPointSampler());
                else if (m_demo == 2)
                    params = m_normals2x.createSampledView(ObjectID::DefaultPointSampler());
                cmd.opBindParametersInline("TestParams"_id, params);

                DrawQuad(cmd, m_shaderPreview, -1.0f, -1.0f, 2.0f, 2.0f);
                cmd.opEndPass();
            }
        }

    } // test
} // rendering