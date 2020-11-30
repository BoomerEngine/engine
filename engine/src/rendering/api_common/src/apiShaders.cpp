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

#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingPipeline.h"

namespace rendering
{
    namespace api
    {
		//--

		IBaseShaders::IBaseShaders(IBaseThread* drv, const ShaderData* data)
			: IBaseObject(drv, ObjectType::Shaders)
			, m_sourceMetadata(AddRef(data->metadata()))
			, m_sourceData(data->data())
			, m_mask(data->metadata()->stageMask)
			, m_key(data->metadata()->key)
		{
			// resolve the vertex layout
			if (m_sourceMetadata->stageMask.test(ShaderStage::Vertex))
				m_vertexLayout = drv->objectCache()->resolveVertexBindingLayout(m_sourceMetadata);

			// resolve descriptor state
			m_descriptorLayout = drv->objectCache()->resolveDescriptorBindingLayout(m_sourceMetadata);
		}

		IBaseShaders::~IBaseShaders()
		{}

		//--

		ShadersObjectProxy::ShadersObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const ShaderMetadata* metadata)
			: ShaderObject(id, impl, metadata)
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
