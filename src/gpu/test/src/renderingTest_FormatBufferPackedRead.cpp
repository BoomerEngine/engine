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

/// test of the texture buffer read in the shader
class RenderingTest_FormatBufferPackedRead : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_FormatBufferPackedRead, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferViewDepth) override final;

private:
    GraphicsPipelineObjectPtr m_shaders;
    BufferObjectPtr m_vertexBuffer;
	BufferObjectPtr m_extraBuffer;
	BufferViewPtr m_extraBufferSRV;

    uint32_t m_sideCount;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_FormatBufferPackedRead);
RTTI_METADATA(RenderingTestOrderMetadata).order(300);
RTTI_END_TYPE();

//---       

static void PrepareTestGeometry(float x, float y, float w, float h, Array<Simple3DVertex>& outVertices)
{
    outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x+w, y, 0.5f, 1.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + w, y+h, 0.5f, 1.0f, 1.0f));

    outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));
    outVertices.pushBack(Simple3DVertex(x, y+h, 0.5f, 0.0f, 1.0f));
}

static void PrepareTestTexels(uint32_t sideCount, Array<Color>& outColors)
{
    outColors.reserve(sideCount*sideCount);
    for (uint32_t y = 0; y < sideCount; y++)
    {
        float fy = y / (float)(sideCount - 1);
        for (uint32_t x = 0; x < sideCount; x++)
        {
            float fx = x / (float)(sideCount - 1);
                    
            float f = 0.5f + cos(fy * 4) * sin(fx * 4);
            outColors.pushBack(Color::FromVectorLinear(Vector4(1.0f - f, f, 0.0f, 1.0f)));
        }
    }
}

void RenderingTest_FormatBufferPackedRead::initialize()
{
    m_sideCount = 64;

    // generate test geometry
    {
        Array<Simple3DVertex> vertices;
        PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);
        m_vertexBuffer = createVertexBuffer(vertices);
    }

    // generate colors
    {
        Array<Color> colors;
        PrepareTestTexels(m_sideCount, colors);

        gpu::BufferCreationInfo info;
        info.allowShaderReads = true;
        info.size = colors.dataSize();

        auto sourceData = CreateSourceData(colors);
        m_extraBuffer = createBuffer(info, sourceData);
		m_extraBufferSRV = m_extraBuffer->createView(ImageFormat::RGBA8_UNORM);
    }

    m_shaders = loadGraphicsShader("FormatBufferPackedRead.csl");
}

void RenderingTest_FormatBufferPackedRead::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferViewDepth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

	struct
	{
		uint32_t sideCount;
	} consts;

	consts.sideCount = m_sideCount;

	DescriptorEntry desc[2];
	desc[0].constants(consts);
	desc[1] = m_extraBufferSRV;
    cmd.opBindDescriptor("TestParams"_id, desc);

    cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
    cmd.opDraw(m_shaders, 0, 6); // quad
    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
