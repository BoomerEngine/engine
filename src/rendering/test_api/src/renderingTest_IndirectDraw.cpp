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
#include "rendering/device/include/renderingBuffer.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

///--

class RenderingTest_IndirectDraw : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_IndirectDraw, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
    BufferObjectPtr m_vertexBuffer; // tower
	BufferObjectPtr m_indirectBuffer; // buffer with indirect draws
	BufferWritableStructuredViewPtr m_indirectBufferUAV;

	ComputePipelineObjectPtr m_generateShader;
    GraphicsPipelineObjectPtr m_drawShader;

	static const uint32_t SIZE = 32;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_IndirectDraw);
    RTTI_METADATA(RenderingTestOrderMetadata).order(4100);
RTTI_END_TYPE();

//---       

static void PrepareTestGeometry(uint32_t count, base::Array<Simple3DVertex>& outVertices)
{
    for (uint32_t py = 0; py < count; ++py)
    {
		auto* v = outVertices.allocateUninitialized(6);

		float x0 = 0.25f;
		float x1 = x0 + 0.6f;
		float y0 = py + 0.25f;
		float y1 = y0 + 0.5f;
				
		v[0].set(x0, y0, 0.5f, 0, 0, base::Color::WHITE);
		v[1].set(x1, y0, 0.5f, 0, 0, base::Color::WHITE);
		v[2].set(x1, y1, 0.5f, 0, 0, base::Color::WHITE);
		v[3].set(x0, y0, 0.5f, 0, 0, base::Color::WHITE);
		v[4].set(x1, y1, 0.5f, 0, 0, base::Color::WHITE);
		v[5].set(x0, y1, 0.5f, 0, 0, base::Color::WHITE);
    }
}

void RenderingTest_IndirectDraw::initialize()
{
    {
        base::Array<Simple3DVertex> vertices;
		PrepareTestGeometry(SIZE, vertices);
        m_vertexBuffer = createVertexBuffer(vertices);
    }

	{
		const auto size = sizeof(GPUDrawArguments) * SIZE;
		m_indirectBuffer = createIndirectBuffer(size, sizeof(GPUDrawArguments));
		m_indirectBufferUAV = m_indirectBuffer->createWritableStructuredView();
	}

    m_drawShader = loadGraphicsShader("GenericGeometryScaled.csl");
	m_generateShader = loadComputeShader("ComputeGenerateSimpleDrawIndirect.csl");
}

void RenderingTest_IndirectDraw::render(GPUCommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
	//--

	{
		struct
		{
			float time;
			uint32_t size;
		} consts;

		consts.time = time;
		consts.size = SIZE;

		DescriptorEntry desc[2];
		desc[0].constants(consts);
		desc[1] = m_indirectBufferUAV;
		cmd.opBindDescriptor("TestParams"_id, desc);

		cmd.opDispatchThreads(m_generateShader, SIZE);
	}

	//--

	cmd.opTransitionLayout(m_indirectBuffer, ResourceLayout::UAV, ResourceLayout::IndirectArgument);

	//--

    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);
            
	const auto size = 2.0f / SIZE;

	for (uint32_t i = 0; i < SIZE; ++i)
	{
		base::Vector4 params;
		params.x = size;
		params.y = size;
		params.z = -1.0f + i * size; // offsetX
		params.w = -1.0f; // offsetY

		DescriptorEntry desc[1];
		desc[0].constants(params);
		cmd.opBindDescriptor("TestParams"_id, desc);

		cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);
		//cmd.opDraw(m_drawShader, 0, 6 * SIZE);
		cmd.opDrawIndirect(m_drawShader, m_indirectBuffer, sizeof(GPUDrawArguments) * i);
	}

	cmd.opEndPass();
}

END_BOOMER_NAMESPACE(rendering::test)