/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "gpu/device/include/device.h"
#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

/// test of a static image
class RenderingTest_ImageCopyRect : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ImageCopyRect, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
	ImageObjectPtr m_sourceImage;
	ImageReadOnlyViewPtr m_sourceImageSRV;

	ImageObjectPtr m_displayImage;
	ImageReadOnlyViewPtr m_displayImageSRV;

    GraphicsPipelineObjectPtr m_shaders;

	FastRandState rnd;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_ImageCopyRect);
    RTTI_METADATA(RenderingTestOrderMetadata).order(2006);
    //RTTI_METADATA(RenderingTestSubtestCountMetadata).count(2);
RTTI_END_TYPE();

//---       

void RenderingTest_ImageCopyRect::initialize()
{
	m_sourceImage = loadImage2D("lena.png", false, true);
	m_sourceImageSRV = m_sourceImage->createReadOnlyView();

	m_displayImage = createChecker2D(1024, 64, false, Color::WHITE, Color::LIGHTSLATEGRAY);
	m_displayImageSRV = m_displayImage->createReadOnlyView();

    m_shaders = loadGraphicsShader("TextureLoad.csl");
}

void RenderingTest_ImageCopyRect::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
	//--

	// copy part
	{
		auto w = 16 + rnd.range(64);
		auto h = 16 + rnd.range(64);
		auto sx = rnd.range(m_sourceImage->width() - w);
		auto sy = rnd.range(m_sourceImage->height() - h);
		auto dx = rnd.range(m_displayImage->width() - w);
		auto dy = rnd.range(m_displayImage->height() - h);

		cmd.opTransitionLayout(m_sourceImage, ResourceLayout::ShaderResource, ResourceLayout::CopySource);
		cmd.opTransitionLayout(m_displayImage, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);

		ResourceCopyRange srcRange;
		srcRange.image.mip = 0;
		srcRange.image.slice = 0;
		srcRange.image.offsetX = sx;
		srcRange.image.offsetY = sy;
		srcRange.image.offsetZ = 0;
		srcRange.image.sizeX = w;
		srcRange.image.sizeY = h;
		srcRange.image.sizeZ = 1;

		ResourceCopyRange destRange;
		destRange.image.mip = 0;
		destRange.image.slice = 0;
		destRange.image.offsetX = dx;
		destRange.image.offsetY = dy;
		destRange.image.offsetZ = 0;
		destRange.image.sizeX = w;
		destRange.image.sizeY = h;
		destRange.image.sizeZ = 1;

		cmd.opCopy(m_sourceImage, srcRange, m_displayImage, destRange);

		cmd.opTransitionLayout(m_sourceImage, ResourceLayout::CopySource, ResourceLayout::ShaderResource);
		cmd.opTransitionLayout(m_displayImage, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
	}

	//--

    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

	DescriptorEntry desc[1];
	desc[0] = m_displayImageSRV;
    cmd.opBindDescriptor("TestParams"_id, desc);

	drawQuad(cmd, m_shaders);

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
