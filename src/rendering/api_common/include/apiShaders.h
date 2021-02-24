/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiObject.h"

#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingShader.h"

BEGIN_BOOMER_NAMESPACE(rendering::api)

//---

/// loaded shaders, this object mainly servers as caching interface to object cache
class RENDERING_API_COMMON_API IBaseShaders : public IBaseObject
{
public:
	IBaseShaders(IBaseThread* drv, const ShaderData* data);
	virtual ~IBaseShaders();

	static const auto STATIC_TYPE = ObjectType::Shaders;

	//--

	// what shaders do we have in the bundle
	INLINE ShaderStageMask mask() const { return m_mask; }

	// key that identifies the shader bundle (can be used to find compiled data in cache)
	INLINE uint64_t key() const { return m_key; }

	// get source shader (client-side) metadata
	INLINE const ShaderMetadata* sourceMetadata() const { return m_sourceMetadata; }

	// get source data (portable opcodes)
	INLINE const base::Buffer& sourceData() const { return m_sourceData; }

	// vertex layout to use with shaders (NULL for compute shaders)
	INLINE IBaseVertexBindingLayout* vertexLayout() const { return m_vertexLayout; }

	// descriptor layout to use with shaders
	INLINE IBaseDescriptorBindingLayout* descriptorLayout() const { return m_descriptorLayout; }

	//--

	// create graphical rendering pipeline using these shaders that is compatible with given rendering pass and rendering states
	virtual IBaseGraphicsPipeline* createGraphicsPipeline_ClientApi(const GraphicsRenderStatesSetup& setup) = 0;

	// create graphical rendering pipeline using these shaders that is compatible with given rendering pass and rendering states
	virtual IBaseComputePipeline* createComputePipeline_ClientApi() = 0;

	//--

private:
	uint64_t m_key = 0;
	ShaderStageMask m_mask;

	ShaderMetadataPtr m_sourceMetadata;
	base::Buffer m_sourceData;

	IBaseVertexBindingLayout* m_vertexLayout = nullptr;
	IBaseDescriptorBindingLayout* m_descriptorLayout = nullptr;
};

//---

// client side proxy for shaders object
class RENDERING_API_COMMON_API ShadersObjectProxy : public ShaderObject
{
public:
	ShadersObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const ShaderMetadata* metadata);

	virtual GraphicsPipelineObjectPtr createGraphicsPipeline(const GraphicsRenderStatesObject* renderStats) override;
	virtual ComputePipelineObjectPtr createComputePipeline() override;

private:
	base::SpinLock m_lock;

	base::HashMap<uint64_t, base::RefWeakPtr<GraphicsPipelineObject>> m_pipelineObjectMap;
	base::RefWeakPtr<ComputePipelineObject> m_compilePipelineObject;
};

//---

END_BOOMER_NAMESPACE(rendering::api)