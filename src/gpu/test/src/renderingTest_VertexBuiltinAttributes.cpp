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

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

/// a different vertex data computation
class RenderingTest_VertexBuiltinAttributes : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_VertexBuiltinAttributes, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
    VertexIndexBunch<> m_indexedTriList;
    GraphicsPipelineObjectPtr m_shaders;

    static const auto NUM_ELEMENTS = 32;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_VertexBuiltinAttributes);
RTTI_METADATA(RenderingTestOrderMetadata).order(170);
RTTI_END_TYPE();

//---

static void GenerateTestGeometry(float x, float y, float w, float h, uint32_t count, VertexIndexBunch<>& outGeometry)
{
    float xs = w / (float)count * 0.9f;
    float ys = h / (float)count * 0.9f;

    for (uint16_t i = 0; i < count; ++i)
    {
        float px = x + w * (i / (float)count);
        float py = y;

        outGeometry.m_indices.pushBack(i * 4 + 0);
        outGeometry.m_indices.pushBack(i * 4 + 1);
        outGeometry.m_indices.pushBack(i * 4 + 2);
        outGeometry.m_indices.pushBack(i * 4 + 0);
        outGeometry.m_indices.pushBack(i * 4 + 2);
        outGeometry.m_indices.pushBack(i * 4 + 3);

        outGeometry.m_vertices.pushBack(Simple3DVertex(px, py, 0.5f));
        outGeometry.m_vertices.pushBack(Simple3DVertex(px+xs, py, 0.5f));
        outGeometry.m_vertices.pushBack(Simple3DVertex(px + xs, py+ys, 0.5f));
        outGeometry.m_vertices.pushBack(Simple3DVertex(px, py+ys, 0.5f));
    }
}

void RenderingTest_VertexBuiltinAttributes::initialize()
{
    GenerateTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, NUM_ELEMENTS, m_indexedTriList);
    m_indexedTriList.createBuffers(*this);
    m_shaders = loadGraphicsShader("VertexBuiltinAttributes.csl");
}

void RenderingTest_VertexBuiltinAttributes::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

	struct
	{
		int NumInstances;
		int NumVertices;
		float InstanceStepY;
	} params;

    params.NumInstances = NUM_ELEMENTS;
    params.NumVertices = NUM_ELEMENTS;
    params.InstanceStepY = (1.8f) / (NUM_ELEMENTS + 1);

	DescriptorEntry desc[1];
	desc[0].constants(params);
    cmd.opBindDescriptor("TestParams"_id, desc);

    m_indexedTriList.draw(cmd, m_shaders, 0, NUM_ELEMENTS);

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
