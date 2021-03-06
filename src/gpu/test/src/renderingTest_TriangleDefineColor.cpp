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
class RenderingTest_TriangleDefineColor : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_TriangleDefineColor, IRenderingTest);

public:
    RenderingTest_TriangleDefineColor();

    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

private:
    BufferObjectPtr m_vertexBuffer;
    GraphicsPipelineObjectPtr m_shaderRed;
	GraphicsPipelineObjectPtr m_shaderGreen;
	GraphicsPipelineObjectPtr m_shaderBlue;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_TriangleDefineColor);
    RTTI_METADATA(RenderingTestOrderMetadata).order(11);
RTTI_END_TYPE();

//---       
RenderingTest_TriangleDefineColor::RenderingTest_TriangleDefineColor()
{
}

void RenderingTest_TriangleDefineColor::initialize()
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

    m_shaderRed = loadGraphicsShader("TriangleDefineColorRed.csl");
    m_shaderGreen = loadGraphicsShader("TriangleDefineColorGreen.csl");
    m_shaderBlue = loadGraphicsShader("TriangleDefineColorBlue.csl");
}

void RenderingTest_TriangleDefineColor::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
    cmd.opBindVertexBuffer("Vertex2D"_id, m_vertexBuffer);
    cmd.opDraw(m_shaderRed, 0, 3);
    cmd.opDraw(m_shaderGreen, 3, 3);
    cmd.opDraw(m_shaderBlue, 6, 3);
    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
