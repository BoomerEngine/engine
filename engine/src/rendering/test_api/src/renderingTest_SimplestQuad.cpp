/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

namespace rendering
{
    namespace test
    {
        /// simples quest - all generated in VS, no data passed from engine, isolates data passing pipeline
        class RenderingTest_SimplestQuad : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_SimplestQuad, IRenderingTest);

        public:
            RenderingTest_SimplestQuad();
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) override final;

        private:
            const ShaderLibrary* m_shader;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_SimplestQuad);
            RTTI_METADATA(RenderingTestOrderMetadata).order(6);
        RTTI_END_TYPE();

        //---       

        RenderingTest_SimplestQuad::RenderingTest_SimplestQuad()
        {
        }

        void RenderingTest_SimplestQuad::initialize()
        {
            m_shader = loadShader("SimplestQuad.csl");
        }

        void RenderingTest_SimplestQuad::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);
            cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
            cmd.opDraw(m_shader, 0, 4);
            cmd.opEndPass();
        }

        //---

    } // test
} // rendering