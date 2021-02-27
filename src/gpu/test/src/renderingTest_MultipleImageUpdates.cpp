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
#include "core/image/include/image.h"
#include "core/image/include/imageUtils.h"
#include "core/image/include/imageView.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

/// test of a multiple image updates within one frame
class RenderingTest_MultipleImageUpdates : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_MultipleImageUpdates, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

private:
    static const uint32_t IMAGE_SIZE = 8;

    ImageObjectPtr m_sampledImage;
	ImageSampledViewPtr m_sampledImageSRV;

    BufferObjectPtr m_vertexBuffer;

    GraphicsPipelineObjectPtr m_shaders;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_MultipleImageUpdates);
RTTI_METADATA(RenderingTestOrderMetadata).order(2030);
RTTI_END_TYPE();

//---       

static void PrepareTestGeometry(float x, float y, float w, float h, Array<Simple3DVertex>& outVertices)
{
    outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + w, y, 0.5f, 1.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));

    outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));
    outVertices.pushBack(Simple3DVertex(x, y + h, 0.5f, 0.0f, 1.0f));
}

void RenderingTest_MultipleImageUpdates::initialize()
{
    m_shaders = loadGraphicsShader("MultipleImageUpdates.csl");

    // generate test geometry
    {
        Array<Simple3DVertex> vertices;
        PrepareTestGeometry(0.1f, 0.1f, 0.8f, 0.8f, vertices);
        m_vertexBuffer = createVertexBuffer(vertices);
    }

    ImageCreationInfo info;
    info.allowShaderReads = true;
    info.allowDynamicUpdate = true;
    info.width = IMAGE_SIZE;
    info.height = IMAGE_SIZE;
    info.format = ImageFormat::RGBA8_UNORM;
    m_sampledImage = createImage(info);
	m_sampledImageSRV = m_sampledImage->createSampledView();
}

void RenderingTest_MultipleImageUpdates::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
{
    auto GRID_SIZE = 16;

    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

	// skip ahead some numbers
	FastRandState rng;
	int skipCount = (int)(time * 20);
	for (int i = 0; i < skipCount; ++i)
	{
		rng.next();
		//rng.next();
		//rng.next();
	}

    // draw the grid
    for (uint32_t y = 0; y < GRID_SIZE; ++y)
    {
        for (uint32_t x = 0; x < GRID_SIZE; ++x)
        {
            // setup pos
			struct
			{
				float ox, oy, sx, sy;
			} consts;
            consts.sx = 2.0f / (1 + GRID_SIZE);
            consts.sy = 2.0f / (1 + GRID_SIZE);
            consts.ox = -1.0f + consts.sx * 0.5f + (x * consts.sx);
            consts.oy = -1.0f + consts.sy * 0.5f + (y * consts.sy);

            // setup params
			DescriptorEntry desc[2];
            desc[0].constants(consts);
            desc[1] = m_sampledImageSRV;
            cmd.opBindDescriptor("TestParams"_id, desc);

            // update the content
            {
                // prepare data
                Color updateColors[IMAGE_SIZE*IMAGE_SIZE];
                Color randomColor = Color(rng.next(), rng.next(), rng.next());
                for (uint32_t i=0; i<ARRAY_COUNT(updateColors); ++i)
                    updateColors[i] = randomColor;

                // send update
                image::ImageView updateView(image::NATIVE_LAYOUT, image::PixelFormat::Uint8_Norm, 4, &updateColors, IMAGE_SIZE, IMAGE_SIZE);
				cmd.opTransitionLayout(m_sampledImage, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);
                cmd.opUpdateDynamicImage(m_sampledImage, updateView);
				cmd.opTransitionLayout(m_sampledImage, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
			}

            // draw the quad
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDraw(m_shaders, 0, 6); // quad
        }
    }

    // exit pass
    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
