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
        /// test of the render to texture functionality
        class RenderingTest_RenderToTexture : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_RenderToTexture, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView) override final;

        private:
            SimpleScenePtr m_scene;
            SceneCamera m_camera;
            bool m_showDepth = false;

            const ShaderLibrary* m_shaderDraw;
            const ShaderLibrary* m_shaderPreview;

            ImageView m_colorBuffer;
            ImageView m_depthBuffer;

            static const uint32_t SIZE = 512;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_RenderToTexture);
            RTTI_METADATA(RenderingTestOrderMetadata).order(2500);
            RTTI_METADATA(RenderingTestSubtestCountMetadata).count(2);
        RTTI_END_TYPE();

        //---

        namespace
        {
            struct TestParams
            {
                ImageView TestImage;
            };
        }

        void RenderingTest_RenderToTexture::initialize()
        {
            // load shaders
            m_showDepth = subTestIndex() != 0;
            m_shaderDraw = loadShader("GenericScene.csl");

            if (m_showDepth)
                m_shaderPreview = loadShader("RenderToTexturePreviewDepth.csl");
            else
                m_shaderPreview = loadShader("RenderToTexturePreviewColor.csl");

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
            
            rendering::ImageCreationInfo depthBuffer;
            depthBuffer.allowRenderTarget = true;
            depthBuffer.allowShaderReads = true;
            depthBuffer.format = rendering::ImageFormat::D24S8;
            depthBuffer.width = SIZE;
            depthBuffer.height = SIZE;
            m_depthBuffer = createImage(depthBuffer);
        }

        void RenderingTest_RenderToTexture::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView)
        {
            // render scene
            {
                // setup scene camera
                SceneCamera camera;
                camera.cameraPosition = base::Vector3(-4.5f, 0.5f, 1.5f);
                camera.calcMatrices(true);

                // render shit to render targets
                FrameBuffer fb;
                fb.color[0].view(m_colorBuffer).clear(base::Vector4(0.2f, 0.2f, 0.2f, 1.0f));
                fb.depth.view(m_depthBuffer).clearDepth(1.0f).clearStencil(0.0f);

                cmd.opBeingPass(fb);
                cmd.opSetDepthState();
                m_scene->draw(cmd, m_shaderDraw, camera);
                cmd.opEndPass();
            }

            // we will wait for the buffer to be generated
            cmd.opGraphicsBarrier();

            // transition the data to reading format
            cmd.opImageLayoutBarrier(m_depthBuffer, ImageLayout::ShaderReadOnly);
            cmd.opImageLayoutBarrier(m_colorBuffer, ImageLayout::ShaderReadOnly);

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
            cmd.opBeingPass(fb);
            {
                auto pointSampler = ObjectID::DefaultPointSampler();

                TestParams params;
                params.TestImage = m_showDepth ? m_depthBuffer.createSampledView(pointSampler) : m_colorBuffer;
                cmd.opBindParametersInline("TestParams"_id, params);
                DrawQuad(cmd, m_shaderPreview, -0.8f, -0.8f, 1.6f, 1.6f);
            }
            cmd.opEndPass();
        }

    } // test
} // rendering