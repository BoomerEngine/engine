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

/// test of multiple vertex streams 
class RenderingTest_MultipleVertexStreams : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_MultipleVertexStreams, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
    BufferObjectPtr m_vertexBuffer0;
	BufferObjectPtr m_vertexBuffer1;
	BufferObjectPtr m_indexBuffer;
    GraphicsPipelineObjectPtr m_shaders;

    uint32_t m_indexCount;
    uint32_t m_vertexCount;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_MultipleVertexStreams);
RTTI_METADATA(RenderingTestOrderMetadata).order(160);
RTTI_END_TYPE();

//---       

struct VertexStream0
{
    Vector3 VertexPosition;

    INLINE VertexStream0()
    {}

    INLINE VertexStream0(float x, float y, float z)
        : VertexPosition(x,y,z)
    {}
};

struct VertexStream1
{
    Color VertexColor;

    INLINE VertexStream1()
    {}

    INLINE VertexStream1(Color color)
        : VertexColor(color)
    {}
};

static void PrepareTestGeometry(float x, float y, float w, float h, Array<uint16_t>& outIndices, Array<VertexStream0>& outVertices0, Array<VertexStream1>& outVertices1)
{
    static const uint32_t NUM_TRIANGLES_START = 64;

    uint32_t rowCount = NUM_TRIANGLES_START;
    float rowStepX = w / (float)NUM_TRIANGLES_START;
    float rowStepY = h / (float)NUM_TRIANGLES_START;
    float rowStartX = x;
    float rowPosY = y;
    while (rowCount > 0)
    {
        float curX = rowStartX;
        float halfX = rowStartX + rowStepX * 0.5f;
        float nextX = rowStartX + rowStepX;
        float nextY = rowPosY + rowStepY;
        uint32_t i = 0;
        while (i < rowCount)
        {
            // odd triangle
            {
                auto base = range_cast<uint16_t>(outVertices0.size());
                outIndices.pushBack(base + 0);
                outIndices.pushBack(base + 1);
                outIndices.pushBack(base + 2);

                Color color = Color::FromVectorLinear(Vector4(1.0f - i / (float)NUM_TRIANGLES_START, 0, 0, 1));
                outVertices0.pushBack(VertexStream0(curX, rowPosY, 0.5f));
                outVertices0.pushBack(VertexStream0(nextX, rowPosY, 0.5f));
                outVertices0.pushBack(VertexStream0(halfX, nextY, 0.5f));
                outVertices1.pushBack(color);
                outVertices1.pushBack(color);
                outVertices1.pushBack(color);
            }

            // even triangle
            if (i > 0)
            {
                auto base = range_cast<uint16_t>(outVertices0.size());
                outIndices.pushBack(base + 0);
                outIndices.pushBack(base + 1);
                outIndices.pushBack(base + 2);

                Color color = Color::FromVectorLinear(Vector4(0, rowCount / (float)NUM_TRIANGLES_START, 0, 1));
                outVertices0.pushBack(VertexStream0(curX, rowPosY, 0.5f));
                outVertices0.pushBack(VertexStream0(halfX, nextY, 0.5f));
                outVertices0.pushBack(VertexStream0(halfX - rowStepX, nextY, 0.5f));
                outVertices1.pushBack(color);
                outVertices1.pushBack(color);
                outVertices1.pushBack(color);
            }
                    
            i += 1;
            halfX += rowStepX;
            curX += rowStepX;
            nextX += rowStepX;
        }

        rowPosY += rowStepY;
        rowStartX += rowStepX * 0.5f;
        rowCount -= 1;
    }
}

void RenderingTest_MultipleVertexStreams::initialize()
{
    // generate test geometry
    Array<uint16_t> indices;
    Array<VertexStream0> vertices0;
    Array<VertexStream1> vertices1;
    PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, indices, vertices0, vertices1);

    m_vertexCount = vertices0.size();

    m_indexCount = indices.size();
    m_indexBuffer = createIndexBuffer(indices);

    m_vertexBuffer0 = createVertexBuffer(vertices0);
    m_vertexBuffer1 = createVertexBuffer(vertices1);

    m_shaders = loadGraphicsShader("MultipleVertexStreams.csl");
}

void RenderingTest_MultipleVertexStreams::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

    cmd.opBindIndexBuffer(m_indexBuffer);
    cmd.opBindVertexBuffer("VertexStream0"_id,  m_vertexBuffer0);
    cmd.opBindVertexBuffer("VertexStream1"_id, m_vertexBuffer1);
    cmd.opDrawIndexed(m_shaders, 0, 0, m_indexCount);

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
