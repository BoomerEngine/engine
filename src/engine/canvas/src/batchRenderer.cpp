/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "batchRenderer.h"

#include "gpu/device/include/shaderData.h"
#include "gpu/device/include/pipeline.h"
#include "gpu/device/include/shaderData.h"
#include "gpu/device/include/shader.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/image.h"
#include "gpu/device/include/buffer.h"
#include "gpu/device/include/globalObjects.h"

BEGIN_BOOMER_NAMESPACE_EX(canvas)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasBatchRenderer);
RTTI_END_TYPE();

ICanvasBatchRenderer::~ICanvasBatchRenderer()
{}
		
//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasSimpleBatchRenderer);
RTTI_END_TYPE();

ICanvasSimpleBatchRenderer::ICanvasSimpleBatchRenderer()
{
	m_reloadNotifier = [this]() { loadShaders(); };
}

ICanvasSimpleBatchRenderer::~ICanvasSimpleBatchRenderer()
{}

static bool LoadShader(const gpu::ShaderDataPtr& file, gpu::ShaderObjectPtr& outShader)
{
	return file ? file->deviceShader() : nullptr;
}

void ICanvasSimpleBatchRenderer::loadShaders()
{
	{
		auto shader = gpu::LoadStaticShaderDeviceObject("canvas/canvas_mask.fx");
		m_mask = shader->createGraphicsPipeline(m_renderStates->m_mask);
	}

    // load main shader
	{
		auto shader = loadMainShaderFile();

		for (int i = 0; i < CanvasRenderStates::MAX_BLEND_OPS; ++i)
		{
			m_standardFill[i] = shader->createGraphicsPipeline(m_renderStates->m_standardFill[i]);
			m_maskedFill[i] = shader->createGraphicsPipeline(m_renderStates->m_maskedFill[i]);
		}
	}
}

bool ICanvasSimpleBatchRenderer::initialize(const CanvasRenderStates& renderStates, gpu::IDevice* drv)
{
	m_renderStates = &renderStates;
	loadShaders();
	return true;
}

const gpu::GraphicsPipelineObject* ICanvasSimpleBatchRenderer::selectShader(gpu::CommandWriter& cmd, const RenderData& data) const
{
	switch (data.batchType)
	{
	case canvas::BatchType::ConcaveMask:
		return m_mask;

	case canvas::BatchType::FillConcave:
		cmd.opSetStencilReferenceValue(1);
		return m_maskedFill[(int)data.blendOp];

	case canvas::BatchType::FillConvex:
		return m_standardFill[(int)data.blendOp];
	}

	return nullptr;
}

void ICanvasSimpleBatchRenderer::render(gpu::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const
{
	if (const auto* shader = selectShader(cmd, data))
		cmd.opDraw(shader, firstVertex, numVertices);
}
//--

END_BOOMER_NAMESPACE_EX(canvas)
