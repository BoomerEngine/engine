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
        /// test of 3d mesh rendering
        class RenderingTest_Scene : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_Scene, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            SimpleScenePtr m_scene;
            ImageView m_depthBuffer;
            ImageView m_shadowMap;

            const ShaderLibrary* m_shaderDrawToShadowMap;
            const ShaderLibrary* m_shaderScene;

            uint32_t m_shadowMode = 0;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_Scene);
            RTTI_METADATA(RenderingTestOrderMetadata).order(3000);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(4);
        RTTI_END_TYPE();

        ///---

        void RenderingTest_Scene::initialize()
        {
            m_shadowMode = subTestIndex();
            m_scene = CreatePlatonicScene(*this);

            m_shaderDrawToShadowMap = loadShader("GenericScene.csl");

            if (m_shadowMode == 0)
                m_shaderScene = loadShader("GenericScene.csl");
            else if (m_shadowMode == 1)
                m_shaderScene = loadShader("GenericScenePointShadows.csl");
            else if (m_shadowMode == 2)
                m_shaderScene = loadShader("GenericSceneLinearShadows.csl");
            else if (m_shadowMode == 3)
                m_shaderScene = loadShader("GenericScenePoisonShadows.csl");
        }

        struct ShadowMapParams
        {
            base::Matrix WorldToShadowmap;
        };

        struct ShadowMapDescriptor
        {
            ConstantsView Constants;
            ImageView ShadowMap;
        };

        void RenderingTest_Scene::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            // create depth render target
            if (m_depthBuffer.empty() || m_depthBuffer.width() != backBufferView.width() || m_depthBuffer.height() != backBufferView.height())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::D24S8;
                info.width = backBufferView.width();
                info.height = backBufferView.height();
                m_depthBuffer = createImage(info);
            }

            // create shadowmap
            if (m_shadowMap.empty())
            {
                ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.allowShaderReads = true;
                info.format = rendering::ImageFormat::D32;
                info.width = 1024;
                info.height = 1024;
                m_shadowMap = createImage(info);
            }

            // animate the light
            static float angle = 0.0f;
            angle += 0.03f;
            m_scene->m_lightPosition = base::Angles(-40.0f, angle, 0.0f).forward();

            // render shadowmap
            if (m_shadowMode != 0)
            {
                SceneCamera camera;
                camera.aspect = 1.0f;
                camera.fov = 0.0f;
                camera.zoom = 10.0f;
                camera.cameraTarget = base::Vector3(0,0,0);
                camera.cameraPosition = m_scene->m_lightPosition * 10.0f;
                camera.calcMatrices();

                {
                    FrameBuffer fb;
                    fb.depth.view(m_shadowMap).clearDepth(1.0f).clearStencil(0);

                    cmd.opBeingPass(fb);
                    cmd.opSetDepthState();
                    cmd.opSetDepthBias(500, 1);
                    m_scene->draw(cmd, m_shaderDrawToShadowMap, camera);
                    cmd.opSetDepthBias(DepthBiasState());
                    cmd.opEndPass();
                }

                // we will wait for the buffer to be generated
                cmd.opGraphicsBarrier();

                // transition the data to reading format
                cmd.opImageLayoutBarrier(m_shadowMap, ImageLayout::ShaderReadOnly);

                auto depthCompareSampler = (m_shadowMode == 1)
                    ? rendering::ObjectID::DefaultDepthPointSampler()
                    : rendering::ObjectID::DefaultDepthBilinearSampler();

                auto biasMatrix = base::Matrix(
                    0.5f, 0.0f, 0.0f, 0.5f,
                    0.0f, -0.5f, 0.0f, 0.5f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f
                );

                ShadowMapParams params;
                params.WorldToShadowmap = camera.m_WorldToScreen * biasMatrix;

                ShadowMapDescriptor desc;
                desc.Constants = cmd.opUploadConstants(params);
                desc.ShadowMap = m_shadowMap.createSampledView(depthCompareSampler);
                cmd.opBindParametersInline("SceneShadowMapParams"_id, desc);
            }

            // render scene
            {
                SceneCamera camera;
                camera.aspect = backBufferView.width() / (float) backBufferView.height();
                camera.cameraPosition = base::Vector3(-4.5f, 0.5f, 1.5f);
                camera.calcMatrices();

                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(0.0f, 0.0f, 0.2f, 1.0f);
                fb.depth.view(m_depthBuffer).clearDepth(1.0f).clearStencil(0);

                cmd.opBeingPass(fb);
                cmd.opSetDepthState();
                m_scene->draw(cmd, m_shaderScene, camera);
                cmd.opEndPass();
            }
        }

    } // test
} // rendering