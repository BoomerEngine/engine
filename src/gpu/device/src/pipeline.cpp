/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\pipeline #]
***/

#include "build.h"
#include "pipeline.h"
#include "shaderMetadata.h"
#include "shader.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(GraphicsRenderStatesObject);
RTTI_END_TYPE();

GraphicsRenderStatesObject::GraphicsRenderStatesObject(ObjectID id, IDeviceObjectHandler* impl, const GraphicsRenderStatesSetup& states, uint64_t key)
	: IDeviceObject(id, impl)
	, m_states(states)
	, m_key(key)
{}

GraphicsRenderStatesObject::~GraphicsRenderStatesObject()
{}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(GraphicsPipelineObject);
RTTI_END_TYPE();

GraphicsPipelineObject::GraphicsPipelineObject(ObjectID id, IDeviceObjectHandler* impl, const GraphicsRenderStatesObject* renderStats, const ShaderObject* shaders)
	: IDeviceObject(id, impl)
	, m_renderStats(AddRef(renderStats))
	, m_shaders(AddRef(shaders))
{}

GraphicsPipelineObject::~GraphicsPipelineObject()
{}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ComputePipelineObject);
RTTI_END_TYPE();

ComputePipelineObject::ComputePipelineObject(ObjectID id, IDeviceObjectHandler* impl, const ShaderObject* shaders)
	: IDeviceObject(id, impl)
	, m_shaders(AddRef(shaders))
{
	m_computeGroupX = std::max<uint32_t>(1, shaders->metadata()->computeGroupSizeX);
	m_computeGroupY = std::max<uint32_t>(1, shaders->metadata()->computeGroupSizeY);
	m_computeGroupZ = std::max<uint32_t>(1, shaders->metadata()->computeGroupSizeZ);
}

ComputePipelineObject::~ComputePipelineObject()
{}

//--

END_BOOMER_NAMESPACE_EX(gpu)
