/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingBuffer.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

/// test of a dynamic buffers
class RenderingTest_DynamicBufferUpdate : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_DynamicBufferUpdate, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView) override final;

private:
    static const uint32_t MAX_VERTICES = 1024;

    BufferObjectPtr m_vertexColorBuffer;
	BufferObjectPtr m_vertexBuffer;

    GraphicsPipelineObjectPtr m_shaders;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_DynamicBufferUpdate);
    RTTI_METADATA(RenderingTestOrderMetadata).order(2020);
RTTI_END_TYPE();

//---       

static void PrepareTestGeometry(uint32_t count, float x, float y, float r, float t, base::Vector2* outVertices, base::Color* outColors)
{
    for (uint32_t i=0; i<count; ++i)
    {
        auto frac = (float)i / (float)(count-1);

        // delta
        auto dx = cos(TWOPI * frac);
        auto dy = sin(TWOPI * frac);
        auto displ = r;

        // displacement
        auto speed = 20.0f + 19.0f * sin(t * 0.1f);
        auto displPhase = (cos(frac * speed) + 1.0f);
        displ += displPhase * r;

        // noise
        float noise = rand() / (float)RAND_MAX;
        displ += noise * 0.1f;

        // create vertex
        outVertices[i].x = x + dx * displ;
        outVertices[i].y = y + dy * displ;

        // calc color
        outColors[i] = base::Lerp(base::Color(base::Color::RED), base::Color(base::Color::GREEN), (0.5f + 0.5f*displPhase));
    }
}

void RenderingTest_DynamicBufferUpdate::initialize()
{
    m_vertexBuffer = createVertexBuffer(sizeof(base::Vector2) * MAX_VERTICES, nullptr);
    m_vertexColorBuffer = createVertexBuffer(sizeof(base::Color) * MAX_VERTICES, nullptr);

	GraphicsRenderStatesSetup setup;
	setup.primitiveTopology(PrimitiveTopology::LineStrip);

    m_shaders = loadGraphicsShader("GenericGeometryTwoStreams.csl", &setup);
}

void RenderingTest_DynamicBufferUpdate::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView)
{
    // prepare some dynamic geometry
    {
		cmd.opTransitionLayout(m_vertexBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);
		cmd.opTransitionLayout(m_vertexColorBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);

        auto* dynamicVerticesData = cmd.opUpdateDynamicBufferPtrN<base::Vector2>(m_vertexBuffer, 0, MAX_VERTICES);
        auto* dynamicColorsData = cmd.opUpdateDynamicBufferPtrN<base::Color>(m_vertexColorBuffer, 0, MAX_VERTICES);
        PrepareTestGeometry(MAX_VERTICES, 0.0f, 0.0f, 0.33f, time, dynamicVerticesData, dynamicColorsData);

		cmd.opTransitionLayout(m_vertexBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);
		cmd.opTransitionLayout(m_vertexColorBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);
    }

    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
    cmd.opBindVertexBuffer("VertexStream0"_id,  m_vertexBuffer);
    cmd.opBindVertexBuffer("VertexStream1"_id, m_vertexColorBuffer);
    cmd.opDraw(m_shaders, 0, MAX_VERTICES); // loop
    cmd.opEndPass();
}

END_BOOMER_NAMESPACE(rendering::test)