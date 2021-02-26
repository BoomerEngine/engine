/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "gpu/device/include/renderingDeviceApi.h"
#include "gpu/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

/// random clear color - tests just the swap
class RenderingTest_Clear : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_Clear, IRenderingTest);

public:
    RenderingTest_Clear();
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

    FastRandState m_random;
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

void RenderingTest_Clear::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    Vector4 clearColor;
    clearColor.x = m_random.unit();
    clearColor.y = m_random.unit();
    clearColor.z = m_random.unit();
    clearColor.w = 1.0f;

    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(clearColor);
            
    cmd.opBeingPass(fb);
    cmd.opEndPass();
}

//---

END_BOOMER_NAMESPACE_EX(gpu::test)
