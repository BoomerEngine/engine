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

///--

#pragma pack(push)
#pragma pack(4)
struct GPUChunkInfo
{
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	float sizeX = 1.0f;
	float sizeY = 1.0f;

	uint32_t firstIndex;
	uint32_t numTriangles; // 3 indices per triangle starting at firstIndex
	uint32_t firstVertex; // added to each index
	Color color;
};
#pragma pack(pop)

///--

class RenderingTest_IndirectDrawIndexed : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_IndirectDrawIndexed, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth) override final;

private:
	static const uint32_t VERTS_PER_SIDE = 8; // 9*9 = 81 vertices, 8*8 = 64 quads = 128 triangles
	static const uint32_t QUADS_PER_SIDE = 32; // 32*8 = 256 per window

	//--
	// SOURCE

    BufferObjectPtr m_sourceVertexBuffer;
	BufferViewPtr m_sourceVertexBufferSRV;

	BufferObjectPtr m_sourceIndexBuffer;
	BufferViewPtr m_sourceIndexBufferSRV;

	BufferObjectPtr m_geometryChunkBuffer;
	BufferStructuredViewPtr m_geometryChunkBufferSRV;
	uint32_t m_numGeometryChunks = 0;

	//--
	// TEMP

	BufferObjectPtr m_workingHelperBuffer;
	BufferWritableViewPtr m_workingHelperBufferUAV;

	//--
	// OUTPUT

	BufferObjectPtr m_generatedIndexBuffer;
	BufferWritableViewPtr m_generatedIndexBufferUAV;
	BufferViewPtr m_generatedIndexBufferSRV;

	BufferObjectPtr m_generatedInstanceBuffer;
	BufferWritableStructuredViewPtr m_generatedInstanceBufferUAV;
	BufferStructuredViewPtr m_generatedInstanceBufferSRV;

	BufferObjectPtr m_generatedArgumentBuffer;
	BufferWritableStructuredViewPtr m_generatedArgumentBufferUAV;

	//--

	ComputePipelineObjectPtr m_computeGenerateAll;
	ComputePipelineObjectPtr m_computeFinalizeArgs;
	GraphicsPipelineObjectPtr m_draw;

	//--
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_IndirectDrawIndexed);
    RTTI_METADATA(RenderingTestOrderMetadata).order(4120);
RTTI_END_TYPE();

//---       

static void PrepareTestGeometry(uint32_t count, Array<Simple3DVertex>& outVertices, Array<uint32_t>& outIndices)
{
	FastRandState rng;

	// generate grid, "meshlet"
	for (uint32_t py = 0; py <= count; ++py)
	{
		const auto fracY = py / (float)count;
		for (uint32_t px = 0; px <= count; ++px)
		{
			const auto fracX = px / (float)count;

			auto* v = outVertices.allocateUninitialized(1);
			v->VertexPosition.x = fracX;
			v->VertexPosition.y = fracY;
			v->VertexPosition.z = 0.5f;
			v->VertexUV.x = 0.5f;
			v->VertexUV.y = 0.5f;
			v->VertexColor.r = (int)rng.range(200, 255);
			v->VertexColor.g = (int)rng.range(200, 255);
			v->VertexColor.b = (int)rng.range(200, 255);
			v->VertexColor.a = 255;
		}
	}

	// generate triangle index buffer
	for (uint32_t py = 0; py < count; ++py)
	{
		auto topIndex = (py * (count + 1));
		auto bottomIndex = topIndex + (count + 1);

		for (uint32_t px = 0; px < count; ++px, ++topIndex, ++bottomIndex)
		{
			auto* u = outIndices.allocateUninitialized(6);
			u[0] = topIndex + 0;
			u[1] = topIndex + 1;
			u[2] = bottomIndex + 1;
			u[3] = topIndex + 0;
			u[4] = bottomIndex + 1;
			u[5] = bottomIndex + 0;
		}
	}			
}

void RenderingTest_IndirectDrawIndexed::initialize()
{
	FastRandState rng;

	// source data
    {
        Array<Simple3DVertex> vertices;
		Array<uint32_t> indices;
		PrepareTestGeometry(VERTS_PER_SIDE, vertices, indices);

		{
			BufferCreationInfo info;
			info.allowVertex = true;
			info.allowShaderReads = true;
			info.size = vertices.dataSize();
			info.label = "SourceVertexBuffer";
			m_sourceVertexBuffer = createBuffer(info, CreateSourceData(vertices)); // vertices are read by vertex shader
			m_sourceVertexBufferSRV = m_sourceVertexBuffer->createView(ImageFormat::R32F);
		}

		{
			BufferCreationInfo info;
			info.allowShaderReads = true;
			info.allowIndex = true;
			info.size = indices.dataSize();
			info.label = "SourceIndexBuffer";
			m_sourceIndexBuffer = createBuffer(info, CreateSourceData(indices));
			m_sourceIndexBufferSRV = m_sourceIndexBuffer->createView(ImageFormat::R32_UINT);
		}
    }

	{
		Array<GPUChunkInfo> chunks;

		float rectSize = 2.0f / (float)QUADS_PER_SIDE;

		for (uint32_t y = 0; y < QUADS_PER_SIDE; ++y)
		{
			for (uint32_t x = 0; x < QUADS_PER_SIDE; ++x)
			{
				auto& entry = chunks.emplaceBack();
				entry.offsetX = -1.0f + x * rectSize;
				entry.offsetY = -1.0f + y * rectSize;
				entry.sizeX = rectSize;
				entry.sizeY = rectSize;
				entry.numTriangles = VERTS_PER_SIDE * VERTS_PER_SIDE;
				entry.firstIndex = 0;
				entry.firstVertex = 0;
				entry.color.r = (int)rng.range(100, 255);
				entry.color.g = (int)rng.range(100, 255);
				entry.color.b = (int)rng.range(100, 255);
				entry.color.a = 255;
			}
		}

		{
			BufferCreationInfo info;
			info.allowShaderReads = true;
			info.size = chunks.dataSize();
			info.stride = sizeof(GPUChunkInfo);
			info.label = "SourceMeshletInstances";
			m_geometryChunkBuffer = createBuffer(info, CreateSourceData(chunks));
			m_geometryChunkBufferSRV = m_geometryChunkBuffer->createStructuredView();
		}
	}

	// temp data
	{
		BufferCreationInfo info;
		info.allowShaderReads = true;
		info.allowUAV = true;
		info.size = 32 * sizeof(uint32_t);
		info.label = "TempBuffer";
		m_workingHelperBuffer = createBuffer(info);
		m_workingHelperBufferUAV = m_workingHelperBuffer->createWritableView(ImageFormat::R32_UINT);
	}

	// calculate maximum capacity
	uint32_t maxIndices = (QUADS_PER_SIDE * QUADS_PER_SIDE) * (VERTS_PER_SIDE * VERTS_PER_SIDE * 6);

	// create output buffer for indices
	{
		BufferCreationInfo info;
		info.allowIndex = true;
		info.allowUAV = true;
		info.allowShaderReads = true;
		info.size = sizeof(uint32_t) * maxIndices;
		info.label = "TargetIndexBuffer";
		m_generatedIndexBuffer = createBuffer(info);
		m_generatedIndexBufferUAV = m_generatedIndexBuffer->createWritableView(ImageFormat::R32_UINT);
		m_generatedIndexBufferSRV = m_generatedIndexBuffer->createView(ImageFormat::R32_UINT);
	}

	// create output buffer for instance infos
	{
		BufferCreationInfo info;
		info.allowShaderReads = true;
		info.allowUAV = true;
		info.size = sizeof(GPUChunkInfo) * (QUADS_PER_SIDE * QUADS_PER_SIDE);
		info.stride = sizeof(GPUChunkInfo);
		info.label = "TargetInstanceBuffer";
		m_generatedInstanceBuffer = createBuffer(info);
		m_generatedInstanceBufferUAV = m_generatedInstanceBuffer->createWritableStructuredView();
		m_generatedInstanceBufferSRV = m_generatedInstanceBuffer->createStructuredView();
	}

	// generated draw commands
	{
		BufferCreationInfo info;
		info.allowIndirect = true;
		info.allowUAV = true;
		//info.size = sizeof(GPUDrawIndexedArguments);
		//info.stride = sizeof(GPUDrawIndexedArguments);
		info.size = sizeof(GPUDrawArguments);
		info.stride = sizeof(GPUDrawArguments);				
		info.label = "TargetCommandBuffers";
		m_generatedArgumentBuffer = createBuffer(info);
		m_generatedArgumentBufferUAV = m_generatedArgumentBuffer->createWritableStructuredView();
	}

	//---

    m_draw = loadGraphicsShader("SimpleMeshletDraw.csl");
	//m_generateShader = loadComputeShader("ComputeGenerateSimpleDrawIndirect.csl");
	m_computeGenerateAll = loadComputeShader("SimpleMeshletGeneralteAllCS.csl");
	m_computeFinalizeArgs = loadComputeShader("SimpleMeshletPrepareArgsCS.csl");
}

void RenderingTest_IndirectDrawIndexed::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* depth)
{
	//--

	float occluderCenterX = cosf(time * 0.5f) * 0.6f;
	float occluderCenterY = sinf(time * 0.5f) * 0.6f;
	float occluderSize = 0.3f;

	//--

	// clear buffers
	cmd.opClearWritableBuffer(m_generatedIndexBufferUAV);
	cmd.opClearWritableBuffer(m_workingHelperBufferUAV);
	cmd.opTransitionFlushUAV(m_workingHelperBufferUAV);
	cmd.opTransitionFlushUAV(m_generatedIndexBufferUAV);

	//--

	// generate list of all final chunks
	{
		struct
		{
			uint32_t VersPerSize;
			uint32_t TrianglesPerQuad;
			uint32_t QuadsPerSize;
			uint32_t VertexStride;

			float occluderX;
			float occluderY;
			float occluderSize;
			uint32_t _Padding2;

		} consts;

		consts.VersPerSize = VERTS_PER_SIDE;
		consts.TrianglesPerQuad = VERTS_PER_SIDE * VERTS_PER_SIDE * 2;
		consts.QuadsPerSize = QUADS_PER_SIDE;
		consts.VertexStride = sizeof(Simple3DVertex) / sizeof(float);
		consts.occluderX = occluderCenterX;
		consts.occluderY = occluderCenterY;
		consts.occluderSize = occluderSize;

		DescriptorEntry desc[7];
		desc[0].constants(consts);
		desc[1] = m_geometryChunkBufferSRV;
		desc[2] = m_sourceIndexBufferSRV;
		desc[3] = m_sourceVertexBufferSRV;
		desc[4] = m_workingHelperBufferUAV;
		desc[5] = m_generatedInstanceBufferUAV;
		desc[6] = m_generatedIndexBufferUAV;
		cmd.opBindDescriptor("TestParams"_id, desc);

		const auto trianglesPerQuad = (VERTS_PER_SIDE * VERTS_PER_SIDE);
		const auto totalQuads = (QUADS_PER_SIDE * QUADS_PER_SIDE);
		//cmd.opDispatchThreads(m_generateAll, VERTS_PER_SIDE, VERTS_PER_SIDE, totalQuads); // xy - vertex index, z - chunk index
		cmd.opDispatchGroups(m_computeGenerateAll, 1, 1, totalQuads); // xy - vertex index, z - chunk index
	}

	//--

	cmd.opTransitionFlushUAV(m_workingHelperBufferUAV);
			
	//--

	{
		struct
		{
			uint32_t VersPerSize;
		} consts;

		consts.VersPerSize = VERTS_PER_SIDE;

		DescriptorEntry desc[3];
		desc[0].constants(consts);
		desc[1] = m_workingHelperBufferUAV;
		desc[2] = m_generatedArgumentBufferUAV;
		cmd.opBindDescriptor("TestParams"_id, desc);
		cmd.opDispatchGroups(m_computeFinalizeArgs, 1, 1, 1);
	}

	//--

	// transition buffers
	cmd.opTransitionLayout(m_generatedInstanceBuffer, ResourceLayout::UAV, ResourceLayout::ShaderResource);
	cmd.opTransitionLayout(m_generatedArgumentBuffer, ResourceLayout::UAV, ResourceLayout::IndirectArgument);
	cmd.opTransitionLayout(m_generatedIndexBuffer, ResourceLayout::UAV, ResourceLayout::ShaderResource);
	//cmd.opTransitionLayout(m_sourceVertexBuffer, ResourceLayout::ShaderResource, ResourceLayout::ShaderResource);

	//cmd.opTransitionLayout(m_generatedIndexBuffer, ResourceLayout::UAV, ResourceLayout::IndexBuffer);
	//cmd.opTransitionLayout(m_sourceVertexBuffer, ResourceLayout::ShaderResource, ResourceLayout::VertexBuffer);
	//cmd.opTransitionLayout(m_sourceIndexBuffer, ResourceLayout::ShaderResource, ResourceLayout::IndexBuffer);
	//cmd.opTransitionFlushUAV(m_generatedArgumentBufferUAV);
	//cmd.opTransitionFlushUAV(m_generatedInstanceBufferUAV);

	//--

	// draw quads on a list
	{
		FrameBuffer fb;
		fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

		cmd.opBeingPass(fb);

		struct
		{
			uint32_t vertexStride;
		} consts;

		consts.vertexStride = sizeof(Simple3DVertex) / sizeof(uint32_t);

		DescriptorEntry desc[4];
		desc[0].constants(consts);
		desc[1] = m_generatedInstanceBufferSRV;
		desc[2] = m_generatedIndexBufferSRV;
		desc[3] = m_sourceVertexBufferSRV;
		cmd.opBindDescriptor("DrawDesc"_id, desc);

		//cmd.opBindVertexBuffer("Simple3DVertex"_id, m_sourceVertexBuffer);
		//cmd.opBindIndexBuffer(m_sourceIndexBuffer, ImageFormat::R32_UINT);
		//cmd.opBindIndexBuffer(m_generatedIndexBuffer, ImageFormat::R32_UINT);

		cmd.opDrawIndirect(m_draw, m_generatedArgumentBuffer, 0);
		//cmd.opDrawIndexedIndirect(m_draw, m_generatedArgumentBuffer, 0);
		//cmd.opDrawIndexedInstanced(m_draw, 0, 0, VERTS_PER_SIDE * VERTS_PER_SIDE * 6, 0, QUADS_PER_SIDE* QUADS_PER_SIDE);

		cmd.opEndPass();
	}

	//--

}

END_BOOMER_NAMESPACE_EX(gpu::test)
