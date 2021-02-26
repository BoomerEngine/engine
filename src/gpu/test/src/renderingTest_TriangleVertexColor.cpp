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

/// a basic rendering test
class RenderingTest_TriangleVertexColor : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TriangleVertexColor, IRenderingTest);

public:
    RenderingTest_TriangleVertexColor();

    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

private:
    BufferObjectPtr m_vertexBuffer;
    GraphicsPipelineObjectPtr m_shader;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_TriangleVertexColor);
    RTTI_METADATA(RenderingTestOrderMetadata).order(13);
RTTI_END_TYPE();

//---       
RenderingTest_TriangleVertexColor::RenderingTest_TriangleVertexColor()
{
}

void RenderingTest_TriangleVertexColor::initialize()
{
    {
        Simple3DVertex vertices[9];
        vertices[0].set(0, -0.5f, 0.5f, 0, 0, Color::RED);
        vertices[1].set(-0.7f, 0.5f, 0.5f, 0, 0, Color::RED);
        vertices[2].set(0.7f, 0.5f, 0.5f, 0, 0, Color::RED);
        vertices[0].VertexColor *= 0.5f;

        vertices[3].set(0 - 0.2f, -0.5f, 0.2f, 0, 0, Color::GREEN);
        vertices[4].set(-0.7f - 0.2f, 0.5f, 0.2f, 0, 0, Color::GREEN);
        vertices[5].set(0.7f - 0.2f, 0.5f, 0.2f, 0, 0, Color::GREEN);
        vertices[3].VertexColor *= 0.5f;

        vertices[6].set(0 + 0.2f, -0.5f, 0.8f, 0, 0, Color::BLUE);
        vertices[7].set(-0.7f + 0.2f, 0.5f, 0.8f, 0, 0, Color::BLUE);
        vertices[8].set(0.7f + 0.2f, 0.5f, 0.8f, 0, 0, Color::BLUE);
        vertices[6].VertexColor *= 0.5f;

        m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
    }

    m_shader = loadGraphicsShader("SimpleVertexColor.csl");
}

void RenderingTest_TriangleVertexColor::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);
    cmd.opDraw(m_shader, 0, 9);
    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
