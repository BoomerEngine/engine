/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
***/

#pragma once

#include "dx11Shaders.h"

#include "gpu/api_common/include/apiGraphicsPipeline.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::dx11)

///--

class GraphicsPipeline : public IBaseGraphicsPipeline
{
public:
	GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsRenderStatesSetup& mergedRenderStates);
	virtual ~GraphicsPipeline();

	//--

	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
	INLINE const Shaders* shaders() const { return static_cast<const Shaders*>(IBaseGraphicsPipeline::shaders()); }

	//--				

private:
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::dx11)
