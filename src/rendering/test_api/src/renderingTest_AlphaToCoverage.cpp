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

/// alpha to coverage test
class RenderingTest_AlphaToCoverage : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_AlphaToCoverage, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

    GraphicsPipelineObjectPtr m_shaderGenerate;
	GraphicsPipelineObjectPtr m_shaderDraw;
	GraphicsPipelineObjectPtr m_shaderDraw2;

    ImageObjectPtr m_colorBuffer;
	RenderTargetViewPtr m_colorBufferRTV;

	ImageObjectPtr m_resolvedColorBuffer;
	ImageSampledViewPtr m_resolvedColorBufferSRV;

	virtual void describeSubtest(base::IFormatStream& f) override
	{
		f.appendf("MSAAx{}", subTestIndex() / 2);
		if (subTestIndex() & 1)
			f.append(" Dithered");
	}
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_AlphaToCoverage);
    RTTI_METADATA(RenderingTestOrderMetadata).order(1600);
    RTTI_METADATA(RenderingTestSubtestCountMetadata).count(10);
RTTI_END_TYPE();

//---       

void RenderingTest_AlphaToCoverage::initialize()
{
    m_shaderDraw = loadGraphicsShader("AlphaToCoveragePreview.csl");
    m_shaderDraw2 = loadGraphicsShader("AlphaToCoveragePreviewWithBorder.csl");
            
	GraphicsRenderStatesSetup setup;
	setup.alphaToCoverage(true);
	setup.alphaToCoverageDither(subTestIndex() & 1);

	m_shaderGenerate = loadGraphicsShader("AlphaToCoverageGenerate.csl", &setup);
}

void RenderingTest_AlphaToCoverage::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    if (m_colorBuffer.empty())
    {
        ImageCreationInfo info;
        info.allowRenderTarget = true;
        info.format = rendering::ImageFormat::RGBA8_UNORM;
        info.width = backBufferView->width();
        info.height = backBufferView->height();
        info.numSamples = 1 << (subTestIndex()/2);
        m_colorBuffer = createImage(info);
		m_colorBufferRTV = m_colorBuffer->createRenderTargetView();
    }

    if (m_resolvedColorBuffer.empty())
    {
        ImageCreationInfo info;
        info.allowShaderReads = true;
        info.format = rendering::ImageFormat::RGBA8_UNORM;
        info.width = backBufferView->width();
        info.height = backBufferView->height();
        m_resolvedColorBuffer = createImage(info);
		m_resolvedColorBufferSRV = m_resolvedColorBuffer->createSampledView();
    }

    // draw triangles
    {
        FrameBuffer fb;
        fb.color[0].view(m_colorBufferRTV).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

        cmd.opBeingPass(fb);

        float w = 0.8f;
        float h = 0.8f;
        float r = 0.4f;
        for (uint32_t i = 0; i < 12; ++i)
        {
            float s1 = 1.0f + i * 0.025352f;
            float o1 = 0.1212f + i * 0.74747f;

            float s2 = 1.1f + i * 0.029222f;
            float o2 = 0.17871f + i * 0.123123f;

            float x = w * sin(o1 + time * s1);
            float y = h * sin(o2 + time * s2);

            auto color = base::Color::RED;
            if ((i % 3) == 1) 
                color = base::Color::GREEN;
            if ((i % 3) == 2)
                color = base::Color::BLUE;
            drawQuad(cmd, m_shaderGenerate, x - r * 0.5f, y - r * 0.5f, r, r, 0,0,1,1,color);
        }

        cmd.opEndPass();
    }

    //--

	cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::RenderTarget, ResourceLayout::ResolveSource);
	cmd.opTransitionLayout(m_resolvedColorBuffer, ResourceLayout::ShaderResource, ResourceLayout::ResolveDest);
	cmd.opResolve(m_colorBuffer, m_resolvedColorBuffer);
	cmd.opTransitionLayout(m_resolvedColorBuffer, ResourceLayout::ResolveDest, ResourceLayout::ShaderResource);
	cmd.opTransitionLayout(m_colorBuffer, ResourceLayout::ResolveSource, ResourceLayout::RenderTarget);

    //--


    {
        FrameBuffer fb;
        fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));
        cmd.opBeingPass(fb);

        {
            DescriptorEntry desc[1];
            desc[0] = m_resolvedColorBufferSRV;
            cmd.opBindDescriptor("TestParams"_id, desc);
            drawQuad(cmd, m_shaderDraw, -1.0f, -1.0f, 2.0f, 2.0f);
        }

        {
			DescriptorEntry desc[1];
			desc[0] = m_resolvedColorBufferSRV;
			cmd.opBindDescriptor("TestParams"_id, desc);
            drawQuad(cmd, m_shaderDraw2, 0.2f, 0.2f, 0.8f, 0.8f, 0.45f, 0.45f, 0.55f, 0.55f);
        }

        cmd.opEndPass();
    }
}

//--

END_BOOMER_NAMESPACE(rendering::test)