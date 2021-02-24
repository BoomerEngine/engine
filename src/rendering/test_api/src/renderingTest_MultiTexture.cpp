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
#include "rendering/device/include/renderingSamplerState.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

/// test two or more textures :)
class RenderingTest_MultiTexture : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_MultiTexture, IRenderingTest);

public:
    RenderingTest_MultiTexture();
    virtual void initialize() override final;
            
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
    ImageSampledViewPtr m_textureA;
    ImageSampledViewPtr m_textureB;
    ImageSampledViewPtr m_textureC;
    ImageSampledViewPtr m_textureD;

	SamplerObjectPtr m_samplerA;
	SamplerObjectPtr m_samplerB;
	SamplerObjectPtr m_samplerC;
	SamplerObjectPtr m_samplerD;

    GraphicsPipelineObjectPtr m_shaders;

	SamplerObjectPtr createTestSampler(bool point, bool wrap, float bias);
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_MultiTexture);
    RTTI_METADATA(RenderingTestOrderMetadata).order(800);
	RTTI_METADATA(RenderingTestSubtestCountMetadata).count(3);
RTTI_END_TYPE();

//---       

RenderingTest_MultiTexture::RenderingTest_MultiTexture()
{
}

static void PrepareTestGeometry(float x, float y, float w, float h, base::Array<Simple3DVertex>& outVertices)
{
    outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + w, y, 0.5f, 1.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));

    outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));
    outVertices.pushBack(Simple3DVertex(x, y + h, 0.5f, 0.0f, 1.0f));
}

SamplerObjectPtr RenderingTest_MultiTexture::createTestSampler(bool point, bool wrap, float bias=0.0f)
{
	SamplerState state;
	state.magFilter = point ? FilterMode::Nearest : FilterMode::Linear;
	state.minFilter = point ? FilterMode::Nearest : FilterMode::Linear;
	state.mipmapMode = point ? MipmapFilterMode::Nearest : MipmapFilterMode::Linear;
	state.addresModeU = wrap ? AddressMode::Wrap : AddressMode::Clamp;
	state.addresModeV = wrap ? AddressMode::Wrap : AddressMode::Clamp;
	state.addresModeW = wrap ? AddressMode::Wrap : AddressMode::Clamp;
	state.minLod = bias;

	return createSampler(state);
}

void RenderingTest_MultiTexture::initialize()
{
	switch (subTestIndex())
	{
		case 0:
			m_shaders = loadGraphicsShader("MultiTexture.csl");
			m_textureA = loadImage2D("lena.png")->createSampledView();
			m_textureB = createChecker2D(256, 32, false)->createSampledView();
			m_textureC = loadImage2D("sky_back.png")->createSampledView();
			m_textureD = createChecker2D(512, 8, false, base::Color::RED, base::Color::BLUE)->createSampledView();
			break;

		case 1:
			m_shaders = loadGraphicsShader("MultiDynamicSamplers.csl");
			m_textureA = loadImage2D("image2.png", true, true)->createSampledView();
			m_textureB = m_textureA;
			m_textureC = m_textureA;
			m_textureD = m_textureA;

			m_samplerA = createTestSampler(true, true);
			m_samplerB = createTestSampler(false, true);
			m_samplerC = createTestSampler(false, true, 4.0f);
			m_samplerD = createTestSampler(false, false);
			break;

		case 2:
			m_shaders = loadGraphicsShader("MultiStaticSamplers.csl");
			m_textureA = loadImage2D("image2.png", true, true)->createSampledView();
			m_textureB = m_textureA;
			m_textureC = m_textureA;
			m_textureD = m_textureA;
			break;
	}
}

void RenderingTest_MultiTexture::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

	if (subTestIndex() != 1)
	{
		DescriptorEntry desc[4];
		desc[0] = m_textureA;
		desc[1] = m_textureB;
		desc[2] = m_textureC;
		desc[3] = m_textureD;
		cmd.opBindDescriptor("TestParams"_id, desc);
	}
	else
	{
		DescriptorEntry desc[8];
		desc[0] = m_samplerA;
		desc[1] = m_samplerB;
		desc[2] = m_textureA;
		desc[3] = m_textureB;
		desc[4] = m_samplerC;
		desc[5] = m_textureC;
		desc[6] = m_samplerD;
		desc[7] = m_textureD;
		cmd.opBindDescriptor("TestParams"_id, desc);
	}

	drawQuad(cmd, m_shaders);

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE(rendering::test)