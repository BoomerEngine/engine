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
#include "gpu/device/include/renderingBuffer.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

/// test of the early pixel rejection
class RenderingTest_EarlyPixelTestAtomic : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_EarlyPixelTestAtomic, IRenderingTest);

public:
    RenderingTest_EarlyPixelTestAtomic();
    virtual void initialize() override final;
            
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
    static const auto NUM_LINES = 1024;

	//ShaderLibraryPtr m_shaderClear;
	GraphicsPipelineObjectPtr m_shaderOccluder;
	GraphicsPipelineObjectPtr m_shaderGenerateA;
	GraphicsPipelineObjectPtr m_shaderGenerateB;
	GraphicsPipelineObjectPtr m_shaderDraw;
	GraphicsPipelineObjectPtr m_shaderClearVS;

    BufferObjectPtr m_tempLineBuffer;
	BufferObjectPtr m_tempQuadBuffer;

	BufferObjectPtr m_storageBufferA;
	BufferObjectPtr m_storageBufferB;

	BufferViewPtr m_storageBufferA_SRV;
	BufferViewPtr m_storageBufferB_SRV;

	BufferWritableViewPtr m_storageBufferA_UAV;
	BufferWritableViewPtr m_storageBufferB_UAV;

	uint32_t m_clearMode = 0;

    void renderLine(CommandWriter& cmd, const GraphicsPipelineObject* func, Color color) const;
    void renderQuad(CommandWriter& cmd, const GraphicsPipelineObject* func, float x, float y, float z, float w, float h, Color color) const;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_EarlyPixelTestAtomic);
    RTTI_METADATA(RenderingTestOrderMetadata).order(2155);
    RTTI_METADATA(RenderingTestSubtestCountMetadata).count(2);
RTTI_END_TYPE();

//---       

RenderingTest_EarlyPixelTestAtomic::RenderingTest_EarlyPixelTestAtomic()
{
}

void RenderingTest_EarlyPixelTestAtomic::initialize()
{
	{
		GraphicsRenderStatesSetup setup;
		setup.depth(true);
		setup.depthWrite(true);
		setup.depthFunc(CompareOp::Less);
		m_shaderOccluder = loadGraphicsShader("EarlyFragmentTestsOccluder.csl", &setup);
		m_shaderGenerateA = loadGraphicsShader("EarlyFragmentTestsGenerateNormal.csl", &setup);
		m_shaderGenerateB = loadGraphicsShader("EarlyFragmentTestsGenerateEarlyTests.csl", &setup);
	}

	{
		GraphicsRenderStatesSetup setup;
		setup.primitiveTopology(PrimitiveTopology::LineStrip);
		m_shaderDraw = loadGraphicsShader("EarlyFragmentTestsDraw.csl", &setup);
	}

	{
		GraphicsRenderStatesSetup setup;
		setup.primitiveTopology(PrimitiveTopology::LineStrip);
		m_shaderClearVS = loadGraphicsShader("EarlyFragmentTestsClearVS.csl", &setup);
	}

	{
		BufferCreationInfo info;
		//info.allowShaderReads = true;
		info.allowDynamicUpdate = true;
		info.allowVertex = true;
		info.size = NUM_LINES * sizeof(Simple3DVertex);
		info.stride = 0;
		info.label = "TempLineBuffer";
		m_tempLineBuffer = createBuffer(info);
	}
             
    m_tempQuadBuffer = createVertexBuffer(6 * sizeof(Simple3DVertex), nullptr);

    m_storageBufferA = createStorageBuffer(NUM_LINES * 4);
	m_storageBufferA_SRV = m_storageBufferA->createView(ImageFormat::R32_INT);
	m_storageBufferA_UAV = m_storageBufferA->createWritableView(ImageFormat::R32_INT);

    m_storageBufferB = createStorageBuffer(NUM_LINES * 4);
	m_storageBufferB_SRV = m_storageBufferB->createView(ImageFormat::R32_INT);
	m_storageBufferB_UAV = m_storageBufferB->createWritableView(ImageFormat::R32_INT);
}

void RenderingTest_EarlyPixelTestAtomic::renderLine(CommandWriter& cmd, const GraphicsPipelineObject* func, Color color) const
{
	cmd.opTransitionLayout(m_tempLineBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);
    auto writeVertex = cmd.opUpdateDynamicBufferPtrN<Simple3DVertex>(m_tempLineBuffer, 0, NUM_LINES);
    for (uint32_t i=0; i<NUM_LINES; ++i, ++writeVertex)
    {
        float x = i / (float)NUM_LINES;
        writeVertex->set(-1.0f + 2.0f*x, 0.95f, 0.5f, x, 0.2f, color);
    }

	cmd.opTransitionLayout(m_tempLineBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);

    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_tempLineBuffer);
    cmd.opDraw(func, 0, NUM_LINES);
}

void RenderingTest_EarlyPixelTestAtomic::renderQuad(CommandWriter& cmd, const GraphicsPipelineObject* func, float x, float y, float z, float w, float h, Color color) const
{
	cmd.opTransitionLayout(m_tempQuadBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);
    auto* verts = cmd.opUpdateDynamicBufferPtrN<Simple3DVertex>(m_tempQuadBuffer, 0, 6);
    verts[0] = Simple3DVertex(x, y, z, 0.0f, 0.0f, color);
    verts[1] = Simple3DVertex(x + w, y, z, 1.0f, 0.0f, color);
    verts[2] = Simple3DVertex(x + w, y + h, z, 1.0f, 1.0f, color);
    verts[3] = Simple3DVertex(x, y, z, 0.0f, 0.0f, color);
    verts[4] = Simple3DVertex(x + w, y + h, z, 1.0f, 1.0f, color);
    verts[5] = Simple3DVertex(x, y + h, z, 0.0f, 1.0f, color);

	cmd.opTransitionLayout(m_tempQuadBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);

    cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_tempQuadBuffer);
    cmd.opDraw(func, 0, 6);
}

void RenderingTest_EarlyPixelTestAtomic::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));
    fb.depth.view(depth).clearDepth(1.0f).clearStencil(0);

    cmd.opBeingPass(fb);

	// clear
	if (m_clearMode == 0)
	{
		DescriptorEntry desc[2];
		desc[0] = m_storageBufferA_UAV;
		desc[1] = m_storageBufferB_UAV;
		cmd.opBindDescriptor("TestParams"_id, desc);
		renderLine(cmd, m_shaderClearVS, Color::RED);
	}
	else if (m_clearMode == 1)
	{

	}
	else if (m_clearMode == 2)
	{
		cmd.opClearWritableBuffer(m_storageBufferA_UAV);
		cmd.opClearWritableBuffer(m_storageBufferB_UAV);
		cmd.opTransitionFlushUAV(m_storageBufferA_UAV);
		cmd.opTransitionFlushUAV(m_storageBufferB_UAV);
	}

    // draw test rect A - no early tests
    const float h = 0.1f;
    const float y1 = -0.15f;
    const float y2 = 0.05f;

    const float oy = -0.2f;
    const float oh = 0.4f;
    const float x1 = 0.7f * sin(time * 1.0f);
    const float x2 = 0.8f * sin(time * 0.4f);
    const float x3 = 0.9f * sin(time * 0.1f);
    const float w = 0.3f;

    // draw occludes
    {
        renderQuad(cmd, m_shaderOccluder, -0.2f, oy, 0.1f, 0.4f, oh, Color(70, 70, 70, 255));
        renderQuad(cmd, m_shaderOccluder, -0.6f, oy, 0.1f, 0.05f, oh, Color(70, 70, 70, 255));
        renderQuad(cmd, m_shaderOccluder, -0.7f, oy, 0.1f, 0.01f, oh, Color(70, 70, 70, 255));
        renderQuad(cmd, m_shaderOccluder, 0.3f, oy, 0.1f, 0.1f, oh, Color(70, 70, 70, 255));
        renderQuad(cmd, m_shaderOccluder, 0.5f, oy, 0.1f, 0.01f, oh, Color(70, 70, 70, 255));
        renderQuad(cmd, m_shaderOccluder, 0.7f, oy, 0.1f, 0.001f, oh, Color(70, 70, 70, 255));
    }

    {
		DescriptorEntry desc[1];
		desc[0] = m_storageBufferA_UAV;
		cmd.opBindDescriptor("TestParams"_id, desc);
        renderQuad(cmd, m_shaderGenerateA, x1, y1, 0.5f, w, h, Color(255, 0, 0, 255));
        if (subTestIndex() >= 1)
            renderQuad(cmd, m_shaderGenerateA, x2, y1, 0.5f, w, h, Color(255, 0, 0, 255));
		if (subTestIndex() >= 2)
			renderQuad(cmd, m_shaderGenerateA, x3, y1, 0.5f, w, h, Color(255, 0, 0, 255));
    }

    {
		DescriptorEntry desc[1];
		desc[0] = m_storageBufferB_UAV;
		cmd.opBindDescriptor("TestParams"_id, desc);
        renderQuad(cmd, m_shaderGenerateB, x1, y2, 0.5f, w, h*1.01f, Color(0, 255, 0, 255));
        if (subTestIndex() >= 1)
            renderQuad(cmd, m_shaderGenerateB, x2, y2, 0.5f, w, h * 1.01f, Color(0, 255, 0, 255));
        if (subTestIndex() >= 2)
            renderQuad(cmd, m_shaderGenerateB, x3, y2, 0.5f, w, h * 1.01f, Color(0, 255, 0, 255));
    }

    cmd.opTransitionLayout(m_storageBufferA, ResourceLayout::UAV, ResourceLayout::ShaderResource);
	cmd.opTransitionLayout(m_storageBufferB, ResourceLayout::UAV, ResourceLayout::ShaderResource);

    // draw
	{
		DescriptorEntry desc[1];
		desc[0] = m_storageBufferA_SRV;
		cmd.opBindDescriptor("TestParams"_id, desc);
		renderLine(cmd, m_shaderDraw, Color::RED);
	}

	{
		DescriptorEntry desc[1];
		desc[0] = m_storageBufferB_SRV;
		cmd.opBindDescriptor("TestParams"_id, desc);
        renderLine(cmd, m_shaderDraw, Color::GREEN);
    }

	cmd.opTransitionLayout(m_storageBufferA, ResourceLayout::ShaderResource, ResourceLayout::UAV);
	cmd.opTransitionLayout(m_storageBufferB, ResourceLayout::ShaderResource, ResourceLayout::UAV);

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
