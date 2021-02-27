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

/// a basic rendering test
class RenderingTest_ColorMask : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ColorMask, IRenderingTest);

public:
    RenderingTest_ColorMask();

    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

private:
    BufferObjectPtr m_vertexBuffer;

    GraphicsPipelineObjectPtr m_shader[4];
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_ColorMask);
    RTTI_METADATA(RenderingTestOrderMetadata).order(15);
RTTI_END_TYPE();

//---       

RenderingTest_ColorMask::RenderingTest_ColorMask()
{
}

static void GenerateTriangle(float x0, float y0, float w, float h, Array<Simple3DVertex>& outVertices)
{
	float x1 = x0 + w;
	float y1 = y0 + h;

	auto* vertices = outVertices.allocateUninitialized(6);
	vertices[0].set(x0, y0, 0.5f, 0, 0, Color::RED);
	vertices[1].set(x1, y0, 0.5f, 0, 0, Color::GREEN);
	vertices[2].set(x1, y1, 0.5f, 0, 0, Color::BLUE);
	vertices[3].set(x0, y0, 0.5f, 0, 0, Color::CYAN);
	vertices[4].set(x1, y1, 0.5f, 0, 0, Color::MAGENTA);
	vertices[5].set(x0, y1, 0.5f, 0, 0, Color::YELLOW);
}

void RenderingTest_ColorMask::initialize()
{
    {
		Array<Simple3DVertex> vertices;
		vertices.reserve(24);

		GenerateTriangle(-1, -1, 1, 1, vertices);
		GenerateTriangle(0, -1, 1, 1, vertices);
		GenerateTriangle(0, 0, 1, 1, vertices);
		GenerateTriangle(-1, 0, 1, 1, vertices);

		m_vertexBuffer = createVertexBuffer(vertices);
    }

	for (uint32_t i=0; i<4; ++i)
	{
		GraphicsRenderStatesSetup setup;
		setup.colorMask(0, 1U << i);

		m_shader[i] = loadGraphicsShader("SimpleVertexColor.csl", &setup);
	}
}

void RenderingTest_ColorMask::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);

	for (uint32_t i=0; i<4; ++i)
		cmd.opDraw(m_shader[i], i*6, 6);

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
