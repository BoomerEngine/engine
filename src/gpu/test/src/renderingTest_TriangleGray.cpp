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
class RenderingTest_TriangleGray : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TriangleGray, IRenderingTest);

public:
    RenderingTest_TriangleGray();

    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

private:
    BufferObjectPtr m_vertexBuffer;
    GraphicsPipelineObjectPtr m_shader;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_TriangleGray);
    RTTI_METADATA(RenderingTestOrderMetadata).order(9);
RTTI_END_TYPE();

//---       
RenderingTest_TriangleGray::RenderingTest_TriangleGray()
{
}

void RenderingTest_TriangleGray::initialize()
{
    {
        Vector2 vertices[9];
        vertices[0] = Vector2(0, -0.5f);
        vertices[1] = Vector2(-0.7f, 0.5f);
        vertices[2] = Vector2(0.7f, 0.5f);

        vertices[3] = Vector2(0 - 0.2f, -0.5f);
        vertices[4] = Vector2(-0.7f - 0.2f, 0.5f);
        vertices[5] = Vector2(0.7f - 0.2f, 0.5f);

        vertices[6] = Vector2(0 + 0.2f, -0.5f);
        vertices[7] = Vector2(-0.7f + 0.2f, 0.5f);
        vertices[8] = Vector2(0.7f + 0.2f, 0.5f);

        m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
    }

    m_shader = loadGraphicsShader("TriangleGray.csl");
}

void RenderingTest_TriangleGray::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
    cmd.opBindVertexBuffer("Vertex2D"_id, m_vertexBuffer);
    cmd.opDraw(m_shader, 0, 9);
    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
