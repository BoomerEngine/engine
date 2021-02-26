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

/// fill mode test
class RenderingTest_FillMode : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_FillMode, IRenderingTest);

public:
    RenderingTest_FillMode();

    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

private:
    BufferObjectPtr m_vertexBuffer;
    GraphicsPipelineObjectPtr m_shaderPoint;
	GraphicsPipelineObjectPtr m_shaderLine;
	GraphicsPipelineObjectPtr m_shaderFill;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_FillMode);
    RTTI_METADATA(RenderingTestOrderMetadata).order(20);
RTTI_END_TYPE();

//---       

RenderingTest_FillMode::RenderingTest_FillMode()
{
}

static void AddTriangle(float x, float y, float w, float h, Color color, Simple3DVertex*& writePtr)
{
    float hw = w * 0.5f;
    float hh = h * 0.5f;
    writePtr[0].set(x, y - hh, 0.5f, 0, 0, color);
    writePtr[1].set(x - hw, y + hh, 0.5f, 0, 0, color);
    writePtr[2].set(x + hw, y + hh, 0.5f, 0, 0, color);
    writePtr += 3;
}

void RenderingTest_FillMode::initialize()
{
    // create vertex buffer with a single triangle
    {
        Simple3DVertex vertices[3*12];

        Simple3DVertex* writePtr = vertices;
        float x = -0.6f;
        float y = -0.6f;
        AddTriangle(x, y, 0.5f, 0.3f, Color::WHITE, writePtr); x += 0.6f;
        AddTriangle(x, y, 0.5f, 0.3f, Color::WHITE, writePtr); x += 0.6f;
        AddTriangle(x, y, 0.5f, 0.3f, Color::WHITE, writePtr); x += 0.6f;
        x = -0.6f;
        y = 0.0f;
        AddTriangle(x, y, 0.5f, 0.3f, Color::WHITE, writePtr); x += 0.6f;
        AddTriangle(x, y, 0.5f, 0.3f, Color::WHITE, writePtr); x += 0.6f;
        AddTriangle(x, y, 0.5f, 0.3f, Color::WHITE, writePtr); x += 0.6f;
        x = -0.6f;
        y = 0.6f;
        AddTriangle(x, y, 0.5f, 0.3f, Color::WHITE, writePtr); x += 0.6f;
        AddTriangle(x, y, 0.5f, 0.3f, Color::WHITE, writePtr); x += 0.6f;
        AddTriangle(x, y, 0.5f, 0.3f, Color::WHITE, writePtr); x += 0.6f;

        m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
    }

	{
		GraphicsRenderStatesSetup setup;
		setup.fill(FillMode::Point);
		m_shaderPoint = loadGraphicsShader("SimpleVertexColor.csl", &setup);
	}

	{
		GraphicsRenderStatesSetup setup;
		setup.fill(FillMode::Line);
		m_shaderLine = loadGraphicsShader("SimpleVertexColor.csl", &setup);
	}

	{
		GraphicsRenderStatesSetup setup;
		setup.fill(FillMode::Fill);
		m_shaderFill = loadGraphicsShader("SimpleVertexColor.csl", &setup);
	}
}

void RenderingTest_FillMode::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);

    int pos = 0;

    // normal modes
    {
        // solid fill mode
        {
            cmd.opDraw(m_shaderFill, 3 * pos, 3);
            pos += 1;
        }

        // line fill mode
        {
            cmd.opDraw(m_shaderLine, 3 * pos, 3);
            pos += 1;
        }

        // point fill mode
        {
            cmd.opDraw(m_shaderPoint, 3 * pos, 3);
            pos += 1;
        }
    }

    // static line
    {
        // solid fill mode
        {
			cmd.opSetLineWidth(0.5f);
			cmd.opDraw(m_shaderLine, 3 * pos, 3);
            pos += 1;
        }

        // line fill mode
        {
			cmd.opSetLineWidth(1.0f);
			cmd.opDraw(m_shaderLine, 3 * pos, 3);

            pos += 1;
        }

        // point fill mode
        {
			cmd.opSetLineWidth(4.0f);
			cmd.opDraw(m_shaderLine, 3 * pos, 3);

            pos += 1;
        }
    }

    // dynamic line
    {
        // solid fill mode
        {
			cmd.opSetLineWidth(0.5f);
			cmd.opDraw(m_shaderLine, 3 * pos, 3);
            pos += 1;
        }

        // line fill mode
        {
			cmd.opSetLineWidth(5.0f);
			cmd.opDraw(m_shaderLine, 3 * pos, 3);
            pos += 1;
        }

        // point fill mode
        {
			cmd.opSetLineWidth(10.0f);
			cmd.opDraw(m_shaderLine, 3 * pos, 3);
            pos += 1;
        }
    }

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
