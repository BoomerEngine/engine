/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {
        /// test of the texture buffer read in the shader
        class RenderingTest_PixelPosition : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_PixelPosition, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) override final;

        private:
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_PixelPosition);
            RTTI_METADATA(RenderingTestOrderMetadata).order(90);
        RTTI_END_TYPE();

        //---
        
        void RenderingTest_PixelPosition::initialize()
        {
            m_shaders = loadShader("ScreenCoordBorder.csl");
        }

        void RenderingTest_PixelPosition::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            const auto resolution = base::Point(backBufferView.width(), backBufferView.height());
            cmd.opBindParametersInline("TestParams"_id, cmd.opUploadConstants(resolution));

            setQuadParams(cmd, 0.0f, 0.0f, 1.0f, 1.0f);

            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
            cmd.opDraw(m_shaders, 0, 4);
        
            cmd.opEndPass();
        }

    } // test
} // rendering