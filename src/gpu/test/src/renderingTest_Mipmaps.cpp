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

/// test of a implicit lod calculation + mipmaps
class RenderingTest_Mipmaps : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_Mipmaps, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
	BufferObjectPtr m_vertexBuffer;

    ImageObjectPtr m_sampledImage;
	ImageSampledViewPtr m_sampledImageSRV;
    GraphicsPipelineObjectPtr m_shaders;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_Mipmaps);
    RTTI_METADATA(RenderingTestOrderMetadata).order(800);
RTTI_END_TYPE();

//---       

static void AddQuad(float x, float y, float size, Array<Simple3DVertex>& outVertices)
{
    outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + size, y, 0.5f, 1.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + size, y + size, 0.5f, 1.0f, 1.0f));
    outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
    outVertices.pushBack(Simple3DVertex(x + size, y + size, 0.5f, 1.0f, 1.0f));
    outVertices.pushBack(Simple3DVertex(x, y + size, 0.5f, 0.0f, 1.0f));
}

static void PrepareTestGeometry(float x, float y, float w, float h, Array<Simple3DVertex>& outVertices)
{
    auto px = x;
    auto py = y;
    auto m = 0.05f;

    AddQuad(px, py, 2.0f, outVertices);
    px += 0.2f;
    py += 0.2f;

    AddQuad(px, py, 1.0f, outVertices); 
    px += 1.0f + m;

    float size = 0.5f;
    for (uint32_t i = 0; i < 12; ++i)
    {
        AddQuad(px, py, size, outVertices);
        py += size + m;
        size /= 2.0f;
    }
}

void RenderingTest_Mipmaps::initialize()
{
	{
		Array<Simple3DVertex> v;
		PrepareTestGeometry(-1, -1, 2, 2, v);
		m_vertexBuffer = createVertexBuffer(v);
	}

    m_sampledImage = createMipmapTest2D(1024, true);
	m_sampledImageSRV = m_sampledImage->createSampledView();
            
    m_shaders = loadGraphicsShader("GenericTexture.csl");
}

void RenderingTest_Mipmaps::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

	{
		DescriptorEntry desc[1];
		desc[0] = m_sampledImageSRV;
		cmd.opBindDescriptor("TestParams"_id, desc);
	}

    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);
    cmd.opDraw(m_shaders, 0, m_vertexBuffer->size() / sizeof(Simple3DVertex));
    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
