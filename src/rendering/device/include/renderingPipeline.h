/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\pipeline #]
***/

#pragma once

#include "renderingObject.h"
#include "renderingResources.h"
#include "renderingGraphicsStates.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

// graphics states wrapper, object represents baked basic render states
class RENDERING_DEVICE_API GraphicsRenderStatesObject : public IDeviceObject
{
	RTTI_DECLARE_VIRTUAL_CLASS(GraphicsRenderStatesObject, IDeviceObject);

public:
	GraphicsRenderStatesObject(ObjectID id, IDeviceObjectHandler* impl, const GraphicsRenderStatesSetup& states, uint64_t key);
    virtual ~GraphicsRenderStatesObject();

	//--

	// rendering states we used to generate this from
	INLINE const GraphicsRenderStatesSetup& states() const { return m_states; }

	// merged "key" (64-bit hash) that represents the graphical render stats
	INLINE uint64_t key() const { return m_key; }

	//--
	
protected:
	uint64_t m_key = 0;
	GraphicsRenderStatesSetup m_states;
};

///---

/// compiled graphics pipeline - all render settings required to render with shaders into render targets
class RENDERING_DEVICE_API GraphicsPipelineObject : public IDeviceObject
{
	RTTI_DECLARE_VIRTUAL_CLASS(GraphicsPipelineObject, IDeviceObject);

public:
	GraphicsPipelineObject(ObjectID id, IDeviceObjectHandler* impl, const GraphicsRenderStatesObject* renderStats, const ShaderObject* shaders);
	virtual ~GraphicsPipelineObject();

	// render states
	INLINE const GraphicsRenderStatesObjectPtr& renderStats() const { return m_renderStats; }

	// source shaders
	INLINE const ShaderObjectPtr& shaders() const { return m_shaders; }

public:
	GraphicsRenderStatesObjectPtr m_renderStats;
	ShaderObjectPtr m_shaders;
};

///---

/// compiled compute pipeline
class RENDERING_DEVICE_API ComputePipelineObject : public IDeviceObject
{
	RTTI_DECLARE_VIRTUAL_CLASS(ComputePipelineObject, IDeviceObject);

public:
	ComputePipelineObject(ObjectID id, IDeviceObjectHandler* impl, const ShaderObject* shaders);
	virtual ~ComputePipelineObject();

	// group size (used very often so it's exposed here)
	INLINE uint32_t groupSizeX() const { return m_computeGroupX; }
	INLINE uint32_t groupSizeY() const { return m_computeGroupY; }
	INLINE uint32_t groupSizeZ() const { return m_computeGroupZ; }

	// source shaders
	INLINE const ShaderObjectPtr& shaders() const { return m_shaders; }

public:
	ShaderObjectPtr m_shaders;

	uint16_t m_computeGroupX = 1;
	uint16_t m_computeGroupY = 1;
	uint16_t m_computeGroupZ = 1;
};

///---

END_BOOMER_NAMESPACE(rendering)