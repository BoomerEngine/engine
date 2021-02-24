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

BEGIN_BOOMER_NAMESPACE(rendering::test)

/// random clear color in rects - tests if clear respects regions
class RenderingTest_ClearInsidePass : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ClearInsidePass, IRenderingTest);

public:
	RenderingTest_ClearInsidePass();
    virtual void initialize() override final;
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

    base::FastRandState m_random;			
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_ClearInsidePass);
    RTTI_METADATA(RenderingTestOrderMetadata).order(2);
RTTI_END_TYPE();

//---       

RenderingTest_ClearInsidePass::RenderingTest_ClearInsidePass()
{
}

void RenderingTest_ClearInsidePass::initialize()
{
}

void RenderingTest_ClearInsidePass::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).dontCare();
            
    cmd.opBeingPass(fb);

	{
		base::Vector4 clearColor;
		clearColor.x = m_random.unit();
		clearColor.y = m_random.unit();
		clearColor.z = m_random.unit();
		clearColor.w = 1.0f;
		cmd.opClearPassRenderTarget(0, clearColor);
	}

    cmd.opEndPass();
}

//---

END_BOOMER_NAMESPACE(rendering::test)