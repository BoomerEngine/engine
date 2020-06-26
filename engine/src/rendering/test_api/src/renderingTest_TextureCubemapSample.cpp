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

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {
        /// test of a cubemap image
        class RenderingTest_TextureCubemapSample : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TextureCubemapSample, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            ImageView m_cubeA;
            ImageView m_cubeB;
            ImageView m_cubeC;

            const ShaderLibrary* m_shaderDraw;
            const ShaderLibrary* m_shaderReference;
            const ShaderLibrary* m_shaderError;
            const ShaderLibrary* m_shaderDrawY;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_TextureCubemapSample);
        RTTI_METADATA(RenderingTestOrderMetadata).order(900);
        RTTI_END_TYPE();

        //---       

        namespace
        {
            struct TestParams
            {
                ImageView TestTexture;
            };
        }
        
        void RenderingTest_TextureCubemapSample::initialize()
        {
            m_cubeA = createFlatCubemap(64);
            m_cubeB = createColorCubemap(256);
            m_cubeC = loadCubemap("sky");

            m_shaderDraw = loadShader("TextureCubemapSampleDraw.csl");
            m_shaderReference = loadShader("TextureCubemapSampleReference.csl");
            m_shaderError = loadShader("TextureCubemapSampleError.csl");
            m_shaderDrawY = loadShader("TextureCubemapSampleDrawY.csl");
        }

        void RenderingTest_TextureCubemapSample::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            auto height = 0.3f;
            auto margin = 0.05f;
            auto y = -1.0f + margin;

            TestParams tempParams;

            // cube A
            tempParams.TestTexture = m_cubeA;
            cmd.opBindParametersInline("TestParams"_id, tempParams);
            DrawQuad(cmd, m_shaderDraw, -0.8f, y, 1.6f, height); y += height + margin;

            // cube B
            tempParams.TestTexture = m_cubeB;
            cmd.opBindParametersInline("TestParams"_id, tempParams);
            DrawQuad(cmd, m_shaderDraw, -0.8f, y, 1.6f, height); y += height + margin;

            // ref mapping
            DrawQuad(cmd, m_shaderReference, -0.8f, y, 1.6f, height); y += height + margin;

            // error term mapping
            DrawQuad(cmd, m_shaderError, -0.8f, y, 1.6f, height); y += height + margin;

            // custom cubemap
            tempParams.TestTexture = m_cubeC;
            cmd.opBindParametersInline("TestParams"_id, tempParams);
            DrawQuad(cmd, m_shaderDrawY, -0.8f, y, 1.6f, height); y += height + margin;

            cmd.opEndPass();
        }

    } // test
} // rendering