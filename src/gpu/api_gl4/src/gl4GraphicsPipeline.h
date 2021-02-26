/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gl4Shaders.h"
#include "gl4StateCache.h"

#include "gpu/api_common/include/apiGraphicsPipeline.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

///--

class GraphicsPipeline : public IBaseGraphicsPipeline
{
public:
	GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsRenderStatesSetup& mergedRenderStates);
	virtual ~GraphicsPipeline();

	//--

	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
	INLINE Shaders* shaders() const { return const_cast<Shaders*>(static_cast<const Shaders*>(IBaseGraphicsPipeline::shaders())); }

	INLINE const StateValues& staticRenderState() const { return m_staticRenderStates; }
	INLINE StateMask staticRenderStateMask() const { return m_staticRenderStateMask; }
				
	//--				

	bool apply(GLuint& glActiveProgram);

	//--

private:
	StateValues m_staticRenderStates;
	StateMask m_staticRenderStateMask;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
