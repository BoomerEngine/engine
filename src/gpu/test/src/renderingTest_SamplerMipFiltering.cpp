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
#include "gpu/device/include/samplerState.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

/// test of a mipmap filtering mode
class RenderingTest_SamplerMipMapFiltering : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_SamplerMipMapFiltering, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
    GraphicsPipelineObjectPtr m_shaders;
    BufferObjectPtr m_vertexBuffer;

    ImageObjectPtr m_sampledImage;
	ImageSampledViewPtr m_sampledImageSRV;

    SamplerObjectPtr m_samplers[5];
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_SamplerMipMapFiltering);
    RTTI_METADATA(RenderingTestOrderMetadata).order(1030);
RTTI_END_TYPE();

//---       }

static void PrepareTestGeometry(Array<Simple3DVertex>& outVertices)
{
    outVertices.pushBack(Simple3DVertex(-1.0f, 1.0f, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(1.0f, 1.0f, 0.5f, 1.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(1.0f, -1.0f, 0.5f, 1.0f, 1.0f));

    outVertices.pushBack(Simple3DVertex(-1.0f, 1.0f, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(1.0f, -1.0f, 0.5f, 1.0f, 1.0f));
    outVertices.pushBack(Simple3DVertex(-1.0f, -1.0f, 0.5f, 0.0f, 1.0f));
}

void RenderingTest_SamplerMipMapFiltering::initialize()
{
    // generate test geometry
    {
        Array<Simple3DVertex> vertices;
        PrepareTestGeometry(vertices);
        m_vertexBuffer = createVertexBuffer(vertices);
    }

    // create the test image
    m_sampledImage = createChecker2D(128, 32);
	m_sampledImageSRV = m_sampledImage->createSampledView();

    {
        SamplerState info;
        info.mipmapMode = MipmapFilterMode::Nearest;
        info.minFilter = FilterMode::Nearest;
        info.magFilter = FilterMode::Nearest;
		info.addresModeU = AddressMode::Wrap;
		info.addresModeV = AddressMode::Wrap;
		info.maxLod = 0.0f;
        m_samplers[0] = createSampler(info);
    }

    {
        SamplerState info;
        info.mipmapMode = MipmapFilterMode::Linear;
        info.minFilter = FilterMode::Nearest;
        info.magFilter = FilterMode::Nearest;
		info.addresModeU = AddressMode::Wrap;
		info.addresModeV = AddressMode::Wrap;
		m_samplers[1] = createSampler(info);
    }

    {
        SamplerState info;
        info.mipmapMode = MipmapFilterMode::Nearest;
        info.minFilter = FilterMode::Linear;
        info.magFilter = FilterMode::Linear;
		info.addresModeU = AddressMode::Wrap;
		info.addresModeV = AddressMode::Wrap;
		m_samplers[2] = createSampler(info);
    }

	{
		SamplerState info;
		info.mipmapMode = MipmapFilterMode::Linear;
		info.minFilter = FilterMode::Linear;
		info.magFilter = FilterMode::Linear;
		info.addresModeU = AddressMode::Wrap;
		info.addresModeV = AddressMode::Wrap;
		m_samplers[3] = createSampler(info);
	}

    {
        SamplerState info;
        info.mipmapMode = MipmapFilterMode::Linear;
        info.minFilter = FilterMode::Linear;
        info.magFilter = FilterMode::Linear;
		info.addresModeU = AddressMode::Wrap;
		info.addresModeV = AddressMode::Wrap;
        info.maxAnisotropy = 16;
        m_samplers[4] = createSampler(info);
    }

    m_shaders = loadGraphicsShader("SamplerMipMapFiltering.csl");
}

void RenderingTest_SamplerMipMapFiltering::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);

    {
        uint32_t numSteps = 5;
        uint32_t viewportWidth = backBufferView->width() / numSteps;
        uint32_t viewportHeight = backBufferView->height() / numSteps;

        for (uint32_t y = 0; y < numSteps; ++y)
        {
			struct
			{
				float TopScaleY;
			} consts;

            static const float zScales[5] = { 1.0f, 2.0f, 5.0f, 10.0f, 20.0f };
            for (uint32_t x = 0; x < numSteps; ++x)
            {
                consts.TopScaleY = zScales[x];

				DescriptorEntry desc[3];
				desc[0].constants(consts);
				desc[1] = m_samplers[y];
				desc[2] = m_sampledImageSRV;
                cmd.opBindDescriptor("TestParams"_id, desc);

                cmd.opSetViewportRect(0, x * viewportWidth, y * viewportHeight, viewportWidth, viewportHeight);
                cmd.opDraw(m_shaders, 0, m_vertexBuffer->size() / sizeof(Simple3DVertex));
            }
        }
    }

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
