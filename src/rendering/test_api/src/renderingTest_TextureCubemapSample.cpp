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

/// test of a cubemap image
class RenderingTest_TextureCubemapSample : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TextureCubemapSample, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
	ImageSampledViewPtr m_cubeA;
	ImageSampledViewPtr m_cubeB;
	ImageSampledViewPtr m_cubeC;

	GraphicsPipelineObjectPtr m_shaderDraw;
	GraphicsPipelineObjectPtr m_shaderReference;
	GraphicsPipelineObjectPtr m_shaderError;
	GraphicsPipelineObjectPtr m_shaderDrawY;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_TextureCubemapSample);
RTTI_METADATA(RenderingTestOrderMetadata).order(900);
RTTI_END_TYPE();

//---       

void RenderingTest_TextureCubemapSample::initialize()
{
    m_cubeA = createFlatCubemap(64)->createSampledView();
    m_cubeB = createColorCubemap(256)->createSampledView();
    m_cubeC = loadCubemap("sky")->createSampledView();

    m_shaderDraw = loadGraphicsShader("TextureCubemapSampleDraw.csl");
    m_shaderReference = loadGraphicsShader("TextureCubemapSampleReference.csl");
    m_shaderError = loadGraphicsShader("TextureCubemapSampleError.csl");
    m_shaderDrawY = loadGraphicsShader("TextureCubemapSampleDrawY.csl");
}

void RenderingTest_TextureCubemapSample::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

    auto height = 0.3f;
    auto margin = 0.05f;
    auto y = -1.0f + margin;

    // cube A
	{
		DescriptorEntry desc[1];
		desc[0] = m_cubeA;
		cmd.opBindDescriptor("TestParams"_id, desc);
		drawQuad(cmd, m_shaderDraw, -0.8f, y, 1.6f, height); y += height + margin;
	}

    // cube B
	{
		DescriptorEntry desc[1];
		desc[0] = m_cubeB;
		cmd.opBindDescriptor("TestParams"_id, desc);
		drawQuad(cmd, m_shaderDraw, -0.8f, y, 1.6f, height); y += height + margin;
	}

    // ref mapping
	{
		drawQuad(cmd, m_shaderReference, -0.8f, y, 1.6f, height); y += height + margin;
	}

    // error term mapping
	{
		drawQuad(cmd, m_shaderError, -0.8f, y, 1.6f, height); y += height + margin;
	}

    // custom cubemap
	{
		DescriptorEntry desc[1];
		desc[0] = m_cubeC;
		cmd.opBindDescriptor("TestParams"_id, desc);
		drawQuad(cmd, m_shaderDrawY, -0.8f, y, 1.6f, height); y += height + margin;
	}

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE(rendering::test)