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
        /// random clear color - tests just the swap
        class RenderingTest_Clear : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_Clear, IRenderingTest);

        public:
            RenderingTest_Clear();
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) override final;

            base::FastRandState m_random;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_Clear);
            RTTI_METADATA(RenderingTestOrderMetadata).order(1);
        RTTI_END_TYPE();

        //---       

        RenderingTest_Clear::RenderingTest_Clear()
        {
        }

        void RenderingTest_Clear::initialize()
        {
        }

        void RenderingTest_Clear::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView )
        {
            base::Vector4 clearColor;
            clearColor.x = base::RandOne(m_random);
            clearColor.y = base::RandOne(m_random);
            clearColor.z = base::RandOne(m_random);
            clearColor.w = 1.0f;

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(clearColor);
            
            cmd.opBeingPass(fb);
            cmd.opEndPass();
        }

        //---

    } // test
} // rendering