/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderer.h"
#include "batchRenderer.h"

#include "gpu/device/include/resources.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/graphicsStates.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/globalObjects.h"
#include "gpu/device/include/shaderService.h"

#include "engine/canvas/include/canvas.h"

BEGIN_BOOMER_NAMESPACE()

//---

ConfigProperty<bool> cvUseCanvasBatching("Render.Canvas", "UseBatching", true);
ConfigProperty<bool> cvUseCanvasGlyphAtlas("Render.Canvas", "UseGlyphAtlas", true);
ConfigProperty<bool> cvUseCanvasImageAtlas("Render.Canvas", "UseImageAtlas", true);

ConfigProperty<uint32_t> cvMaxVertices("Rendering.Canvas", "MaxVertices", 200000);
ConfigProperty<uint32_t> cvMaxAttributes("Rendering.Canvas", "MaxAttributes", 4096);

//---

/// rendering handler for custom canvas batches
class CanvasDefaultBatchRenderer : public ICanvasSimpleBatchRenderer
{
	RTTI_DECLARE_VIRTUAL_CLASS(CanvasDefaultBatchRenderer, ICanvasSimpleBatchRenderer);

public:
	virtual gpu::ShaderObjectPtr loadMainShaderFile() override final
	{
		return gpu::LoadStaticShaderDeviceObject("canvas/canvas_fill.fx");
	}
};

RTTI_BEGIN_TYPE_CLASS(CanvasDefaultBatchRenderer);
RTTI_END_TYPE();

//--

CanvasRenderer::CanvasRenderer()
{
    m_device = GetService<DeviceService>()->device();

    m_maxBatchVetices = std::max<uint32_t>(4096, cvMaxVertices.get());
    m_maxAttributes = std::max<uint32_t>(1024, cvMaxAttributes.get());

    {
		gpu::BufferCreationInfo info;
        info.allowDynamicUpdate = true;
        info.allowVertex = true;
        info.size = sizeof(CanvasVertex) * m_maxBatchVetices;
        info.label = "CanvasVertices";

        m_sharedVertexBuffer = m_device->createBuffer(info);
    }

    {
		gpu::BufferCreationInfo info;
        info.allowShaderReads = true;
        info.size = sizeof(GPUCanvasImageInfo);
        info.stride = sizeof(GPUCanvasImageInfo);
        info.label = "CanvasEmptyAtlasEntries";
        m_emptyAtlasEntryBuffer = m_device->createBuffer(info);
        m_emptyAtlasEntryBufferSRV = m_emptyAtlasEntryBuffer->createStructuredView();
    }

    {
        gpu::BufferCreationInfo info;
        info.allowShaderReads = true;
        info.allowDynamicUpdate = true;
        info.size = sizeof(CanvasAttributes) * m_maxAttributes;
        info.stride = sizeof(CanvasAttributes);
        info.label = "CanvasAttributesBuffer";
        m_sharedAttributesBuffer = m_device->createBuffer(info);
        m_sharedAttributesBufferSRV = m_sharedAttributesBuffer->createStructuredView();

    }

    createRenderStates();
    createBatchRenderers();
}

CanvasRenderer::~CanvasRenderer()
{
    delete m_renderStates;
    m_renderStates = nullptr;

    m_batchRenderers.clearPtr();
}

//--

void CanvasRenderer::render(gpu::CommandWriter& cmd, const RenderInfo& resources, const Canvas& canvas) const
{
	cmd.opBeginBlock("RenderCanvas");

	float scaleX = canvas.width();
	float scaleY = canvas.height();
	float offsetX = 0;
	float offsetY = 0;

	// TODO: clip!

	Matrix canvasToScreen;
	canvasToScreen.identity();
	canvasToScreen.m[0][0] = 2.0f / scaleX;
	canvasToScreen.m[1][1] = 2.0f / scaleY;
	canvasToScreen.m[0][3] = -1.0f + (offsetX / scaleX) - 0.5f * canvasToScreen.m[0][0];
	canvasToScreen.m[1][3] = -1.0f + (offsetX / scaleX) - 0.5f * canvasToScreen.m[1][1];

	//--

	// upload vertices as is
	const auto numVertices = canvas.m_gatheredVertices.size();
	if (const auto vertexDataSize = sizeof(CanvasVertex) * numVertices)
	{
		cmd.opTransitionLayout(m_sharedVertexBuffer, gpu::ResourceLayout::VertexBuffer, gpu::ResourceLayout::CopyDest);

		auto* writePtr = cmd.opUpdateDynamicBufferPtrN<CanvasVertex>(m_sharedVertexBuffer, 0, numVertices);
		canvas.m_gatheredVertices.copy(writePtr, vertexDataSize);

		cmd.opTransitionLayout(m_sharedVertexBuffer, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::VertexBuffer);
	}

	// upload attributes as is
	const auto numAttributes = canvas.m_gatheredAttributes.size();
	if (const auto attributesDataSize = sizeof(CanvasAttributes) * numAttributes)
	{
		cmd.opTransitionLayout(m_sharedAttributesBuffer, gpu::ResourceLayout::ShaderResource, gpu::ResourceLayout::CopyDest);
		cmd.opUpdateDynamicBuffer(m_sharedAttributesBuffer, 0, attributesDataSize, canvas.m_gatheredAttributes.typedData());
		cmd.opTransitionLayout(m_sharedAttributesBuffer, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::ShaderResource);
	}

	// bind vertices
	cmd.opBindVertexBuffer("CanvasVertex"_id, m_sharedVertexBuffer);

	// bound atlas
	int currentBoundAtlas = -1;

	// draw batches, as many as we can in one go
	bool firstBatch = true;
	const auto* batchPtr = canvas.m_gatheredBatches.typedData();
	const auto* batchEnd = batchPtr + canvas.m_gatheredBatches.size();
	while (batchPtr < batchEnd)
	{
		const auto* curBatchStart = batchPtr;

		// find end of the range
		uint32_t firstDrawVertex = batchPtr->vertexOffset;
		uint32_t numDrawVertices = batchPtr->vertexCount;
		while (++batchPtr < batchEnd)
		{
			if ((batchPtr->atlasIndex && batchPtr->atlasIndex != curBatchStart->atlasIndex)
				|| batchPtr->rendererIndex != curBatchStart->rendererIndex
				|| batchPtr->renderDataOffset != curBatchStart->renderDataOffset
				|| batchPtr->op != curBatchStart->op
				|| batchPtr->type != curBatchStart->type)
				break;

			numDrawVertices += batchPtr->vertexCount;
		}

		// render with selected renderer
		if (const auto* renderer = m_batchRenderers[curBatchStart->rendererIndex])
		{
			ASSERT(firstDrawVertex + numDrawVertices <= numVertices);
			if (numDrawVertices)
			{
				// prepare render states
				ICanvasBatchRenderer::RenderData data;
				if (curBatchStart->atlasIndex > 0 && curBatchStart->atlasIndex < resources.numValidAtlases)
				{
					const auto& atlas = resources.atlases[curBatchStart->atlasIndex];

					data.atlasData = atlas.bufferSRV;
					data.atlasImage = atlas.imageSRV;
				}

				data.blendOp = curBatchStart->op;
				data.batchType = curBatchStart->type;
				data.customData = OffsetPtr(canvas.m_gatheredData.typedData(), curBatchStart->renderDataOffset);
				data.vertexBuffer = m_sharedVertexBuffer;
				data.glyphImage = resources.glyphs.imageSRV ? resources.glyphs.imageSRV : gpu::Globals().TextureArrayWhite;

				// rebind atlas data
				if ((curBatchStart->atlasIndex && currentBoundAtlas != curBatchStart->atlasIndex) || firstBatch)
				{
					struct
					{
						Matrix CanvasToScreen;

						uint32_t width;
						uint32_t height;
						uint32_t padding0;
						uint32_t padding1;
					} consts;

					// setup constants
					consts.width = canvas.width();
					consts.height = canvas.height();
					consts.CanvasToScreen = canvasToScreen;

					// bind data
					gpu::DescriptorEntry desc[5];
					desc[0].constants(consts);
					desc[1] = m_sharedAttributesBufferSRV;
					desc[2] = data.atlasData ? data.atlasData : m_emptyAtlasEntryBufferSRV;
					desc[3] = data.atlasImage ? data.atlasImage : gpu::Globals().TextureArrayWhite;
					desc[4] = data.glyphImage ? data.glyphImage : gpu::Globals().TextureArrayWhite;
					cmd.opBindDescriptor("CanvasDesc"_id, desc);

					firstBatch = false;
				}

				// render the vertices from all the batches in one go
				renderer->render(cmd, data, firstDrawVertex, numDrawVertices);
			}
		}
	}

	cmd.opEndBlock();
}

//--

void CanvasRenderer::createRenderStates()
{
	m_renderStates = new CanvasRenderStates();

	// mask generation
	{
		gpu::GraphicsRenderStatesSetup setup;
		setup.stencil(true);
		setup.blend(true);
		setup.blendFactor(0, gpu::BlendFactor::Zero, gpu::BlendFactor::One);
		setup.stencilFront(gpu::CompareOp::Always, gpu::StencilOp::Keep, gpu::StencilOp::Keep, gpu::StencilOp::IncrementAndWrap);
		setup.stencilBack(gpu::CompareOp::Always, gpu::StencilOp::Keep, gpu::StencilOp::Keep, gpu::StencilOp::DecrementAndWrap);
		m_renderStates->m_mask = m_device->createGraphicsRenderStates(setup);
	}

	// drawing - with mask or without
	for (int i = 0; i < CanvasRenderStates::MAX_BLEND_OPS; ++i)
	{
		const auto op = (CanvasBlendOp)i;

		gpu::GraphicsRenderStatesSetup setup;

		switch (op)
		{
			case CanvasBlendOp::Copy:
				break;

			case CanvasBlendOp::Addtive:
				setup.blend(true);
				setup.blendFactor(0, gpu::BlendFactor::One, gpu::BlendFactor::One);
				break;

			case CanvasBlendOp::AlphaBlend:
				setup.blend(true);
				setup.blendFactor(0, gpu::BlendFactor::SrcAlpha, gpu::BlendFactor::OneMinusSrcAlpha);
				break;

			case CanvasBlendOp::AlphaPremultiplied:
				setup.blend(true);
				setup.blendFactor(0, gpu::BlendFactor::One, gpu::BlendFactor::OneMinusSrcAlpha);
				break;
		}

		m_renderStates->m_standardFill[i] = m_device->createGraphicsRenderStates(setup);

		setup.stencil(true);
		setup.stencilFront(gpu::CompareOp::Equal, gpu::StencilOp::Zero, gpu::StencilOp::Zero, gpu::StencilOp::Zero);
		setup.stencilBack(gpu::CompareOp::Equal, gpu::StencilOp::Zero, gpu::StencilOp::Zero, gpu::StencilOp::Zero);

		m_renderStates->m_maskedFill[i] = m_device->createGraphicsRenderStates(setup);
	}
}

void CanvasRenderer::createBatchRenderers()
{
	Array<SpecificClassType<ICanvasBatchRenderer>> batchHandlerClasses;
	RTTI::GetInstance().enumClasses(batchHandlerClasses);

	static bool assignIndices = true;
	if (assignIndices)
	{
		short index = 1;

		for (const auto& cls : batchHandlerClasses)
			if (cls == CanvasDefaultBatchRenderer::GetStaticClass())
				cls->assignUserIndex(0);
			else
				cls->assignUserIndex(index++);

		assignIndices = false;
	}

	m_batchRenderers.resizeWith(batchHandlerClasses.size() + 1, nullptr);

	for (const auto& cls : batchHandlerClasses)
		if (auto handler = cls->createPointer<ICanvasBatchRenderer>())
			if (handler->initialize(*m_renderStates, m_device))
				m_batchRenderers[cls->userIndex()] = handler;
}

//--

END_BOOMER_NAMESPACE()
