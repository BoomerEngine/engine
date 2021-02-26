/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gpu/api_common/include/apiShaders.h"
#include "nullApiThread.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

/// loaded shaders, this object mainly servers as caching interface to object cache
class Shaders : public IBaseShaders
{
public:
	Shaders(Thread* drv, const ShaderData* data);
	virtual ~Shaders();

	//--

	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

	//--

	virtual IBaseGraphicsPipeline* createGraphicsPipeline_ClientApi(const GraphicsRenderStatesSetup& mergedRenderStates) override;
	virtual IBaseComputePipeline* createComputePipeline_ClientApi() override;

	//--

private:
};

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
