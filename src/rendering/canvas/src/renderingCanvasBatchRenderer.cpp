/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasBatchRenderer.h"

#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingPipeline.h"
#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingShader.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingImage.h"
#include "rendering/device/include/renderingBuffer.h"
#include "rendering/device/include/renderingDeviceGlobalObjects.h"

BEGIN_BOOMER_NAMESPACE(rendering)

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

static bool LoadShader(const ShaderDataPtr& file, ShaderObjectPtr& outShader)
{
	return file ? file->deviceShader() : nullptr;
}

void ICanvasSimpleBatchRenderer::loadShaders()
{
	{
		auto shader = LoadStaticShaderDeviceObject("canvas/canvas_mask.fx");
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

bool ICanvasSimpleBatchRenderer::initialize(const CanvasRenderStates& renderStates, IDevice* drv)
{
	m_renderStates = &renderStates;
	loadShaders();
	return true;
}

const GraphicsPipelineObject* ICanvasSimpleBatchRenderer::selectShader(GPUCommandWriter& cmd, const RenderData& data) const
{
	switch (data.batchType)
	{
	case base::canvas::BatchType::ConcaveMask:
		return m_mask;

	case base::canvas::BatchType::FillConcave:
		cmd.opSetStencilReferenceValue(1);
		return m_maskedFill[(int)data.blendOp];

	case base::canvas::BatchType::FillConvex:
		return m_standardFill[(int)data.blendOp];
	}

	return nullptr;
}

void ICanvasSimpleBatchRenderer::render(GPUCommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const
{
	if (const auto* shader = selectShader(cmd, data))
		cmd.opDraw(shader, firstVertex, numVertices);
}
//--

END_BOOMER_NAMESPACE(rendering)