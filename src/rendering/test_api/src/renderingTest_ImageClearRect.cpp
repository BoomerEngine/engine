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
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/image/include/imageView.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

class RenderingTest_ImageClearRect : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ImageClearRect, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

private:
	static const uint32_t IMAGE_SIZE = 512;

	ImageObjectPtr m_sampledImage;
	ImageSampledViewPtr m_sampledImageSRV;
	ImageWritableViewPtr m_sampledImageUAV;

    GraphicsPipelineObjectPtr m_shaders;

	base::FastRandState rnd;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_ImageClearRect);
    RTTI_METADATA(RenderingTestOrderMetadata).order(2005);
RTTI_END_TYPE();

//---       

void RenderingTest_ImageClearRect::initialize()
{
    ImageCreationInfo info;
    info.allowDynamicUpdate = true; // important for dynamic update
    info.allowShaderReads = true;
	info.allowUAV = true;
	info.initialLayout = ResourceLayout::ShaderResource;
    info.width = IMAGE_SIZE;
    info.height = IMAGE_SIZE;
    info.format = ImageFormat::RGBA8_UNORM;
    m_sampledImage = createImage(info);
	m_sampledImageSRV = m_sampledImage->createSampledView();
	m_sampledImageUAV = m_sampledImage->createWritableView();
            
    m_shaders = loadGraphicsShader("GenericGeometryWithTexture.csl");
}

void RenderingTest_ImageClearRect::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
{
	// clear part
	{
		auto w = rnd.range(IMAGE_SIZE / 64, IMAGE_SIZE / 4);
		auto h = rnd.range(IMAGE_SIZE / 64, IMAGE_SIZE / 4);
		auto x = rnd.range(0U, IMAGE_SIZE - w);
		auto y = rnd.range(0U, IMAGE_SIZE - h);

		base::Color color;
		color.r = rnd.next();
		color.g = rnd.next();
		color.b = rnd.next();
		color.a = 255;

		ResourceClearRect rect[1];
		rect[0].image.offsetX = x;
		rect[0].image.offsetY = y;
		rect[0].image.offsetZ = 0;
		rect[0].image.sizeX = w;
		rect[0].image.sizeY = h;
		rect[0].image.sizeZ = 1;

		cmd.opTransitionLayout(m_sampledImage, ResourceLayout::ShaderResource, ResourceLayout::UAV);
		cmd.opClearWritableImageRects(m_sampledImageUAV, &color, rect, 1);
		cmd.opTransitionLayout(m_sampledImage, ResourceLayout::UAV, ResourceLayout::ShaderResource);
	}

    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

	DescriptorEntry desc[1];
	desc[0] = m_sampledImageSRV;
	cmd.opBindDescriptor("TestParams"_id, desc);

	drawQuad(cmd, m_shaders);

    cmd.opDraw(m_shaders, 0, 6); // quad

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE(rendering::test)