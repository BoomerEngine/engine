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
        /// test of the viewport scaling
        class RenderingTest_Viewport : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_Viewport, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

        private:
            BufferObjectPtr m_vertexBuffer; // quad
            ShaderLibraryPtr m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_Viewport);
        RTTI_METADATA(RenderingTestOrderMetadata).order(120);
        RTTI_END_TYPE();

        //---       

        void RenderingTest_Viewport::initialize()
        {
            m_shaders = loadShader("GenericScreenQuad.csl");
        }

        static void DrawRecursivePattern(command::CommandWriter& cmd, const ShaderLibrary* func, uint32_t depth, uint32_t left, uint32_t top, uint32_t width, uint32_t height)
        {
            auto margin = 4;
            auto halfX = left + width / 2;
            auto halfY = top + height / 2;
            auto sizeX = width / 2 - margin * 2;
            auto sizeY = height / 2 - margin * 2;

            // TOP-LEFT
            cmd.opSetViewportRect(0, left + margin, top + margin, sizeX, sizeY);
            cmd.opDraw(func, 0, 6);

            // TOP-RIGHT
            cmd.opSetViewportRect(0, halfX + margin, top + margin, sizeX, sizeY);
            cmd.opDraw(func, 0, 6);

            // BOTTOM-LEFT
            cmd.opSetViewportRect(0, left + margin, halfY + margin, sizeX, sizeY);
            cmd.opDraw(func, 0, 6);

            // BOTTOM-RIGHT
            if (depth < 5)
            {
                DrawRecursivePattern(cmd, func, depth + 1, halfX, halfY, width / 2, height / 2);
            }
            else
            {
                cmd.opSetViewportRect(0, halfX + margin, halfY + margin, sizeX, sizeY);
                cmd.opDraw(func, 0, 6);
            }
        }

        void RenderingTest_Viewport::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.5f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);

            {
                auto minX = 0;
                auto minY = 0;
                auto width = 1024;
                auto height = 1024;

                setQuadParams(cmd, 0.0f, 0.0f, 1.0f, 1.0f);
                DrawRecursivePattern(cmd, m_shaders, 0, minX, minY, width, height);
            }

            cmd.opEndPass();
        }

    } // test
} // rendering