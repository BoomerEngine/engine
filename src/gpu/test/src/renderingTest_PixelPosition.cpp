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

/// test of the texture buffer read in the shader
class RenderingTest_PixelPosition : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_PixelPosition, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

private:
    GraphicsPipelineObjectPtr m_shaders;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_PixelPosition);
    RTTI_METADATA(RenderingTestOrderMetadata).order(90);
RTTI_END_TYPE();

//---
        
void RenderingTest_PixelPosition::initialize()
{
    m_shaders = loadGraphicsShader("ScreenCoordBorder.csl");
}

void RenderingTest_PixelPosition::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

	DescriptorEntry params[1];
	params[0].constants(Point(backBufferView->width(), backBufferView->height()));
	cmd.opBindDescriptor("TestParams"_id, params);

    setQuadParams(cmd, 0.0f, 0.0f, 1.0f, 1.0f);

    cmd.opDraw(m_shaders, 0, 4);
        
    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
