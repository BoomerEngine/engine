/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasRenderer.h"
#include "renderingCanvasBatchRenderer.h"

#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingPipeline.h"
#include "rendering/device/include/renderingShaderFile.h"
#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingShader.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingImage.h"
#include "rendering/device/include/renderingBuffer.h"
#include "rendering/device/include/renderingDeviceGlobalObjects.h"

namespace rendering
{
    namespace canvas
    {
        //---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasBatchRenderer);
        RTTI_END_TYPE();

        ICanvasBatchRenderer::~ICanvasBatchRenderer()
        {}
		
        //---

		base::res::StaticResource<ShaderFile> resCanvasShaderMask("/engine/shaders/canvas/canvas_mask.fx");

		RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasSimpleBatchRenderer);
		RTTI_END_TYPE();

		ICanvasSimpleBatchRenderer::ICanvasSimpleBatchRenderer()
		{}

		ICanvasSimpleBatchRenderer::~ICanvasSimpleBatchRenderer()
		{}

		static bool LoadShader(const ShaderFilePtr& file, ShaderObjectPtr& outShader)
		{
			if (file)
			{
				if (auto data = file->rootShader())
				{
					if (auto dev = data->deviceShader())
					{
						outShader = dev;
						return true;
					}
				}
			}

			return false;
		}

		bool ICanvasSimpleBatchRenderer::initialize(CanvasStorage* owner, IDevice* drv)
		{
			// load mask shader
			if (!LoadShader(resCanvasShaderMask.loadAndGet(), m_shaderMask))
				return false;

			// load main shader
			ShaderDataPtr ptr;
			if (!LoadShader(loadMainShaderFile(), m_shaderFill))
				return false;

			return true;
		}

		void ICanvasSimpleBatchRenderer::prepareForLayout(const CanvasRenderStates& renderStates, GraphicsPassLayoutObject* pass)
		{
			if (m_activeShaderGroup && m_activeShaderGroup->layoutKey == pass->key())
				return;

			for (const auto& shaders : m_shaderGroups)
			{
				if (shaders.layoutKey == pass->key())
				{
					m_activeShaderGroup = &shaders;
					return;
				}
			}

			auto& group = m_shaderGroups.emplaceBack();
			group.layoutKey = pass->key();
			group.m_mask = m_shaderMask->createGraphicsPipeline(pass, renderStates.m_mask);

			for (int i = 0; i < CanvasRenderStates::MAX_BLEND_OPS; ++i)
			{
				group.m_standardFill[i] = m_shaderFill->createGraphicsPipeline(pass, renderStates.m_standardFill[i]);
				group.m_maskedFill[i] = m_shaderFill->createGraphicsPipeline(pass, renderStates.m_maskedFill[i]);
			}

			m_activeShaderGroup = &group;
		}


		const GraphicsPipelineObject* ICanvasSimpleBatchRenderer::ShaderGroup::selectShader(command::CommandWriter& cmd, const RenderData& data) const
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

		void ICanvasSimpleBatchRenderer::render(command::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const
		{
			if (const auto* shader = m_activeShaderGroup->selectShader(cmd, data))
				cmd.opDraw(shader, firstVertex, numVertices);
		}
		//--

    } // canvas
} // rendering