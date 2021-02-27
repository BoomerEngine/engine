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
#include "gpu/device/include/buffer.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

namespace
{
	struct TestConsts
	{
		static const uint32_t ELEMENTS_PER_SIDE = 32;
		static const uint32_t MAX_BUFFER_ELEMENTS = (ELEMENTS_PER_SIDE + 1) * (ELEMENTS_PER_SIDE + 1);

		struct TestElement
		{
			float m_size;
			Vector4 m_color;
		};

		float TestSizeScale;
		TestElement TestElements[MAX_BUFFER_ELEMENTS];
	};
}

/// test of the custom uniform buffer read in the shader
class RenderingTest_UniformBufferRead : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_UniformBufferRead, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
    BufferObjectPtr m_vertexBuffer;
	BufferObjectPtr m_constantBuffer;
    GraphicsPipelineObjectPtr m_shaders;

	BufferConstantViewPtr m_constantView;

    uint32_t m_vertexCount;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_UniformBufferRead);
    RTTI_METADATA(RenderingTestOrderMetadata).order(190);
RTTI_END_TYPE();

//---       

static void PrepareTestGeometry(float x, float y, float w, float h, uint32_t count, Array<Simple3DVertex>& outVertices, TestConsts& outBufferData)
{
    uint32_t index = 0;
    for (uint32_t py = 0; py <= count; ++py)
    {
        auto fy = py / (float)count;
        for (uint32_t px = 0; px <= count; ++px)
        {
            auto fx = px / (float)count;

            auto d = Vector2(fx - 1.5f, fy - 1.5f).length();
            auto s = 1.0f + 7.0f + 7.0f * sin(d * TWOPI * 3.0f);

            Simple3DVertex t;
            t.VertexPosition.x = x + w * fx;
            t.VertexPosition.y = y + h * fy;
            t.VertexPosition.z = 0.5f;
            outVertices.pushBack(t);

            if (index < ARRAY_COUNT(outBufferData.TestElements))
            {
                auto& outElem = outBufferData.TestElements[index++];
                outElem.m_size = s;
                outElem.m_color = Vector4(fy, 0, fx, 1);
            }
        }
    }
}

void RenderingTest_UniformBufferRead::initialize()
{
	TestConsts constData;
	constData.TestSizeScale = 1.0f;

    {
        Array<Simple3DVertex> vertices;
        PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, TestConsts::ELEMENTS_PER_SIDE, vertices, constData);
        m_vertexCount = vertices.size();
        m_vertexBuffer = createVertexBuffer(vertices);
    }

	{
		m_constantBuffer = createConstantBuffer(sizeof(constData), &constData);
		m_constantView = m_constantBuffer->createConstantView();
	}

	GraphicsRenderStatesSetup setup;
	setup.primitiveTopology(PrimitiveTopology::PointList);

    m_shaders = loadGraphicsShader("UniformBufferRead.csl", &setup);
}

void RenderingTest_UniformBufferRead::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
            
	DescriptorEntry desc[1];
	desc[0] = m_constantView;
    cmd.opBindDescriptor("TestParams"_id, desc);

    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);
    cmd.opDraw(m_shaders, 0, m_vertexCount);

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
