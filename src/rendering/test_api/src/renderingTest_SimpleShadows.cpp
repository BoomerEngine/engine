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
        /// test of 3d mesh rendering
        class RenderingTest_SimpleShadows : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_SimpleShadows, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

			virtual void queryInitialCamera(base::Vector3& outPosition, base::Angles& outRotation)
			{
				outPosition = base::Vector3(-4.5f, 0.5f, 1.5f);
				outRotation = base::Angles(15.0f, 0.0f, 0.0f);
			}

			virtual void describeSubtest(base::IFormatStream& f) override
			{
				switch (subTestIndex())
				{
				case 0: f << "NoShadows"; break;
				case 1: f << "Point"; break;
				case 2: f << "PCF"; break;
				case 3: f << "Poison"; break;
				}
			}

        private:
            SimpleScenePtr m_scene;

			ImageObjectPtr m_shadowMap;
			RenderTargetViewPtr m_shadowMapRTV;
			ImageSampledViewPtr m_shadowMapSRV;

            GraphicsPipelineObjectPtr m_shaderDrawToShadowMap;
			GraphicsPipelineObjectPtr m_shaderScene;

            uint32_t m_shadowMode = 0;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_SimpleShadows);
            RTTI_METADATA(RenderingTestOrderMetadata).order(3000);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(4);
        RTTI_END_TYPE();

        ///---

        void RenderingTest_SimpleShadows::initialize()
        {
            m_shadowMode = subTestIndex();
            m_scene = CreatePlatonicScene(*this);

			{
				GraphicsRenderStatesSetup render;
				render.depth(true);
				render.depthWrite(true);
				render.depthFunc(CompareOp::Less);
				render.cull(false);
				render.depthBias(true);
				render.depthBiasValue(500);
				render.depthBiasSlope(1.0f);

				m_shaderDrawToShadowMap = loadGraphicsShader("GenericScene.csl", &render);
			}

			{
				GraphicsRenderStatesSetup render;
				render.depth(true);
				render.depthWrite(true);
				render.depthFunc(CompareOp::LessEqual);

				if (m_shadowMode == 0)
					m_shaderScene = loadGraphicsShader("GenericScene.csl", &render);
				else if (m_shadowMode == 1)
					m_shaderScene = loadGraphicsShader("GenericScenePointShadows.csl", &render);
				else if (m_shadowMode == 2)
					m_shaderScene = loadGraphicsShader("GenericSceneLinearShadows.csl", &render);
				else if (m_shadowMode == 3)
					m_shaderScene = loadGraphicsShader("GenericScenePoisonShadows.csl", &render);
			}
        }

        struct ShadowMapParams
        {
            base::Matrix WorldToShadowmap;
        };
        
        void RenderingTest_SimpleShadows::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepth)
        {
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
				m_shadowMapRTV = m_shadowMap->createRenderTargetView();
				m_shadowMapSRV = m_shadowMap->createSampledView();
            }

            // animate the light
            static float angle = 0.0f;
            angle += 0.03f;
            m_scene->m_lightPosition = base::Angles(-40.0f, angle, 0.0f).forward();

            // render shadowmap
            if (m_shadowMode != 0)
            {
                SceneCamera camera;
				camera.setupDirectionalShadowmap(base::Vector3(0, 0, 0), m_scene->m_lightPosition);
				camera.calcMatrices();

                {
                    FrameBuffer fb;
                    fb.depth.view(m_shadowMapRTV).clearDepth(1.0f).clearStencil(0);

                    cmd.opBeingPass(fb);
                    m_scene->draw(cmd, m_shaderDrawToShadowMap, camera);
                    cmd.opEndPass();
                }

				cmd.opTransitionLayout(m_shadowMap, ResourceLayout::DepthWrite, ResourceLayout::ShaderResource);

                auto depthCompareSampler = (m_shadowMode == 1)
                    ? Globals().SamplerPointDepthLE
                    : Globals().SamplerBiLinearDepthLE;

                auto biasMatrix = base::Matrix(
                    0.5f, 0.0f, 0.0f, 0.5f,
                    0.0f, 0.5f, 0.0f, 0.5f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f
                );

                ShadowMapParams params;
                params.WorldToShadowmap = camera.m_WorldToScreen * biasMatrix;

				DescriptorEntry desc[3];
				desc[0].constants(params);
				desc[1] = depthCompareSampler;
				desc[2] = m_shadowMapSRV;
				//desc[3] = m_shadowMapSRV;				
                cmd.opBindDescriptor("SceneShadowMapParams"_id, desc);
            }

            // render scene
            {
                SceneCamera camera;
                camera.aspect = backBufferView->width() / (float) backBufferView->height();
				camera.position = m_cameraPosition;
				camera.rotation = m_cameraAngles.toQuat();
                camera.calcMatrices(backBufferView->flipped());

                FrameBuffer fb;
                fb.color[0].view(backBufferView).clear(0.0f, 0.0f, 0.2f, 1.0f);
                fb.depth.view(backBufferDepth).clearDepth(1.0f).clearStencil(0);

                cmd.opBeingPass(fb);
                m_scene->draw(cmd, m_shaderScene, camera);
                cmd.opEndPass();
            }
        }

    } // test
} // rendering