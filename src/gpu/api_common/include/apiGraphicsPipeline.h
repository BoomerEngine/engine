/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiObject.h"
#include "gpu/device/include/renderingGraphicsStates.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//---

/// graphics pipeline setup - all you need to draw (besides dynamic states...)
class GPU_API_COMMON_API IBaseGraphicsPipeline : public IBaseObject
{
public:
	IBaseGraphicsPipeline(IBaseThread* owner, const IBaseShaders* shaders, const GraphicsRenderStatesSetup& mergedRenderStates);
	virtual ~IBaseGraphicsPipeline();

	//--

	static const auto STATIC_TYPE = ObjectType::GraphicsPipelineObject;

	//--

	// internal unique key (can be used to find compiled data in cache)
	INLINE uint64_t key() const { return m_key; }

	// render state key
	INLINE uint64_t mergedRenderStateKey() const { return m_mergedRenderStatesKey; }

	// shaders used to build the pipeline
	INLINE const IBaseShaders* shaders() const { return m_shaders; }

	// rendering states
	INLINE const GraphicsRenderStatesSetup& mergedRenderStates() const { return m_mergedRenderStates; }

	//--

private:
	uint64_t m_key = 0;
			
	const IBaseShaders* m_shaders = nullptr;
			
	GraphicsRenderStatesSetup m_mergedRenderStates;
	uint64_t m_mergedRenderStatesKey;
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api)
