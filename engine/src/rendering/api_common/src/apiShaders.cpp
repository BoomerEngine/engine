/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiShaders.h"
#include "apiObjectCache.h"
#include "apiThread.h"
#include "apiGraphicsPipeline.h"
#include "apiComputePipeline.h"
#include "apiGraphicsPassLayout.h"
#include "apiGraphicsRenderStates.h"

#include "rendering/device/include/renderingShaderLibrary.h"
#include "rendering/device/include/renderingPipeline.h"

namespace rendering
{
    namespace api
    {
		//--

		IBaseShaders::IBaseShaders(IBaseThread* drv, const ShaderLibraryData* data, PipelineIndex index)
			: IBaseObject(drv, ObjectType::Shaders)
			, m_data(AddRef(data))
			, m_index(index)
		{
			// resolve the vertex layout
			const auto& shaderBundle = m_data->shaderBundles()[index];
			if (shaderBundle.vertexBindingState != INVALID_PIPELINE_INDEX)
				m_vertexLayout = drv->objectCache()->resolveVertexBindingLayout(*data, shaderBundle.vertexBindingState);

			// set mask for valid shaders
			for (uint32_t i = 0; i < shaderBundle.numShaders; ++i)
			{
				const auto shaderIndex = data->indirectIndices()[shaderBundle.firstShaderIndex + i];
				const auto& shaderEntry = data->shaderBlobs()[shaderIndex];
				m_mask.set(shaderEntry.type);
			}

			// resolve descriptor state
			m_descriptorLayout = drv->objectCache()->resolveDescriptorBindingLayout(*data, shaderBundle.parameterBindingState);

			// data key
			m_key = shaderBundle.bundleKey;
		}

		IBaseShaders::~IBaseShaders()
		{}

		//--

		ShadersObjectProxy::ShadersObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const ShaderLibraryData* data, PipelineIndex index)
			: ShaderObject(id, impl, data, index)
		{}

		GraphicsPipelineObjectPtr ShadersObjectProxy::createGraphicsPipeline(const GraphicsPassLayoutObject* passLayout, const GraphicsRenderStatesObject* renderStats)
		{
			DEBUG_CHECK_RETURN_V(passLayout != nullptr, nullptr);
			DEBUG_CHECK_RETURN_V(renderStats != nullptr, nullptr);

			auto resolvedPassLayout = passLayout->resolveInternalApiObject<IBaseGraphicsPassLayout>();
			DEBUG_CHECK_RETURN_V(resolvedPassLayout != nullptr, nullptr);

			auto resolvedRenderStates = renderStats->resolveInternalApiObject<IBaseGraphicsRenderStates>();
			DEBUG_CHECK_RETURN_V(resolvedRenderStates != nullptr, nullptr);

			if (auto* obj = resolveInternalApiObject<IBaseShaders>())
				if (auto* view = obj->createGraphicsPipeline_ClientApi(resolvedPassLayout, resolvedRenderStates))
					return base::RefNew<rendering::GraphicsPipelineObject>(view->handle(), owner(), passLayout, renderStats, this);

			return nullptr;
		}

		ComputePipelineObjectPtr ShadersObjectProxy::createComputePipeline()
		{
			if (auto* obj = resolveInternalApiObject<IBaseShaders>())
				if (auto* view = obj->createComputePipeline_ClientApi())
					return base::RefNew<rendering::ComputePipelineObject>(view->handle(), owner(), this);

			return nullptr;
		}

		//--

    } // api
} // rendering
