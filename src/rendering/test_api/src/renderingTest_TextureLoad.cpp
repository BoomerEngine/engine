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

/// test of a static image
class RenderingTest_TextureLoad : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TextureLoad, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
	ImageReadOnlyViewPtr m_staticImage;
    GraphicsPipelineObjectPtr m_shaders;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_TextureLoad);
    RTTI_METADATA(RenderingTestOrderMetadata).order(700);
    RTTI_METADATA(RenderingTestSubtestCountMetadata).count(2);
RTTI_END_TYPE();

//---       

void RenderingTest_TextureLoad::initialize()
{
	m_staticImage = loadImage2D("lena.png", false, true)->createReadOnlyView();

    if (subTestIndex() == 0)
        m_shaders = loadGraphicsShader("TextureLoad.csl");
    else
        m_shaders = loadGraphicsShader("TextureLoadOffset.csl");
}

void RenderingTest_TextureLoad::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

	DescriptorEntry desc[1];
	desc[0] = m_staticImage;
    cmd.opBindDescriptor("TestParams"_id, desc);

	drawQuad(cmd, m_shaders, -0.8f, -0.8f, 1.6f, 1.6f);

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE(rendering::test)