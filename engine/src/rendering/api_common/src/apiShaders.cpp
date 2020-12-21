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
#include "apiGraphicsRenderStates.h"

#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingPipeline.h"
#include "rendering/device/include/renderingShaderMetadata.h"

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

		GraphicsPipelineObjectPtr ShadersObjectProxy::createGraphicsPipeline(const GraphicsRenderStatesObject* renderStats)
		{
			GraphicsRenderStatesSetup mergedStates = metadata()->renderStates;

			if (renderStats)
			{
				if (auto apiRenderStates = renderStats->resolveInternalApiObject<IBaseGraphicsRenderStates>())
					mergedStates.apply(apiRenderStates->setup());
			}

			const auto key = mergedStates.key();

			auto lock = CreateLock(m_lock);

			base::RefWeakPtr<GraphicsPipelineObject> psoRef;
			if (m_pipelineObjectMap.find(key, psoRef))
				if (auto pso = psoRef.lock())
					return pso;

			if (auto* obj = resolveInternalApiObject<IBaseShaders>())
			{
				if (auto* view = obj->createGraphicsPipeline_ClientApi(mergedStates))
				{
					auto ret = base::RefNew<rendering::GraphicsPipelineObject>(view->handle(), owner(), renderStats, this);
					m_pipelineObjectMap[key] = ret;
					return ret;
				}
			}

			return nullptr;
		}

		ComputePipelineObjectPtr ShadersObjectProxy::createComputePipeline()
		{
			auto lock = CreateLock(m_lock);

			if (auto ret = m_compilePipelineObject.lock())
				return ret;

			if (auto* obj = resolveInternalApiObject<IBaseShaders>())
			{
				if (auto* view = obj->createComputePipeline_ClientApi())
				{
					auto ret = base::RefNew<rendering::ComputePipelineObject>(view->handle(), owner(), this);
					m_compilePipelineObject = ret;
					return ret;
				}
			}

			return nullptr;
		}

		//--

    } // api
} // rendering
