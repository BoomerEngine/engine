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

/// test of the viuewport depth-range setting 
class RenderingTest_ViewportDepthRange : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_ViewportDepthRange, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
    BufferObjectPtr m_vertexBuffer;

    ImageObjectPtr m_depthBuffer;
	RenderTargetViewPtr m_depthBufferRTV;

    GraphicsPipelineObjectPtr m_shaders;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_ViewportDepthRange);
RTTI_METADATA(RenderingTestOrderMetadata).order(130);
RTTI_END_TYPE();

//---

void RenderingTest_ViewportDepthRange::initialize()
{
    // create vertex buffer with a test geometry
    {
        Simple3DVertex vertices[6];
        vertices[0].set(0.0f, 0.0f, 0.0f);
        vertices[1].set(1.0f, 0.0f, 0.0f);
        vertices[2].set(1.0f, 1.0f, 0.0f);
        vertices[3].set(0.0f, 0.0f, 0.0f);
        vertices[4].set(1.0f, 1.0f, 0.0f);
        vertices[5].set(0.0f, 1.0f, 0.0f);
        m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
    }

	GraphicsRenderStatesSetup setup;
	setup.depth(true);
	setup.depthWrite(true);
	setup.depthFunc(CompareOp::LessEqual);

    m_shaders = loadGraphicsShader("ViewportDepthRange.csl", &setup);
}

static void DrawGroup(CommandWriter& cmd, const GraphicsPipelineObject* func, float startX, float y, float startZ, float endZ, float size, uint32_t count, const Vector4& color)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        float frac = i / (float)(count-1);

		struct
		{
			Vector4 DrawColor;
			Vector4 Scale;
			Vector4 Offset;
		} params;

        params.DrawColor = color;
        params.Scale.x = size;
        params.Scale.y = size;
        params.Scale.z = 0.0f;
        params.Offset.x = startX + frac * (1.6f);
        params.Offset.y = y;
        params.Offset.z = startZ + frac * (endZ - startZ);

        DescriptorEntry desc[1];
		desc[0].constants(params);
        cmd.opBindDescriptor("TestParams"_id, desc);

        cmd.opDraw(func, 0, 6);
    }
}

void RenderingTest_ViewportDepthRange::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));
    fb.depth.view(backBufferDepthView).clearDepth(1.0f).clearStencil(0);

	Rect viewportRect;
	viewportRect.min.x = 0;
	viewportRect.min.y = 0;
	viewportRect.max.x = backBufferView->width();
	viewportRect.max.y = backBufferView->height();

    cmd.opBeingPass(fb);

    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);

    // normal Z
    float y = -0.9f;
    float x = -1.0f;
    {
        cmd.opSetViewportRect(0, viewportRect, 0.0f, 1.0f);
        DrawGroup(cmd, m_shaders, x + 0.0f, y + 0.00f, 0.03f, 0.93f, 0.4f, 4, Vector4::EX());
        DrawGroup(cmd, m_shaders, x + 0.2f, y + 0.05f, 0.06f, 0.96f, 0.3f, 4, Vector4::EY());
        DrawGroup(cmd, m_shaders, x + 0.4f, y + 0.10f, 0.09f, 0.99f, 0.2f, 4, Vector4::EZ());
        y += 0.5f;
    }

    // inverted Z
    {
		cmd.opSetViewportRect(0, viewportRect, 1.0f, 0.0f);
        DrawGroup(cmd, m_shaders, x + 0.0f, y + 0.00f, 0.03f, 0.93f, 0.4f, 4, Vector4::EX());
        DrawGroup(cmd, m_shaders, x + 0.2f, y + 0.05f, 0.06f, 0.96f, 0.3f, 4, Vector4::EY());
        DrawGroup(cmd, m_shaders, x + 0.4f, y + 0.10f, 0.09f, 0.99f, 0.2f, 4, Vector4::EZ());
        y += 0.5f;
    }

    // sorted Z ranges
    {
		cmd.opSetViewportRect(0, viewportRect, 0.0f, 0.3f);
        DrawGroup(cmd, m_shaders, x + 0.0f, y + 0.00f, 0.03f, 0.93f, 0.4f, 4, Vector4::EX());
		cmd.opSetViewportRect(0, viewportRect, 0.3f, 0.6f);
        DrawGroup(cmd, m_shaders, x + 0.2f, y + 0.05f, 0.06f, 0.96f, 0.3f, 4, Vector4::EY());
		cmd.opSetViewportRect(0, viewportRect, 0.6f, 1.0f);
        DrawGroup(cmd, m_shaders, x + 0.4f, y + 0.10f, 0.09f, 0.99f, 0.2f, 4, Vector4::EZ());
        y += 0.5f;
    }

    // inverted Z ranges
    {
		cmd.opSetViewportRect(0, viewportRect, 0.6f, 1.0f);
        DrawGroup(cmd, m_shaders, x + 0.0f, y + 0.00f, 0.03f, 0.93f, 0.4f, 4, Vector4::EX());
		cmd.opSetViewportRect(0, viewportRect, 0.3f, 0.6f);
        DrawGroup(cmd, m_shaders, x + 0.2f, y + 0.05f, 0.06f, 0.96f, 0.3f, 4, Vector4::EY());
		cmd.opSetViewportRect(0, viewportRect, 0.0f, 0.3f);
        DrawGroup(cmd, m_shaders, x + 0.4f, y + 0.10f, 0.09f, 0.99f, 0.2f, 4, Vector4::EZ());
        y += 0.5f;
    }

    cmd.opEndPass();
}

//--

END_BOOMER_NAMESPACE_EX(gpu::test)
