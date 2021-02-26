/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gpu/api_common/include/apiComputePipeline.h"

#include "nullApiThread.h"
#include "nullApiShaders.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

///---

class ComputePipeline : public IBaseComputePipeline
{
public:
	ComputePipeline(Thread* owner, const Shaders* shaders);
	virtual ~ComputePipeline();

	//--

	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
	INLINE const Shaders* shaders() const { return static_cast<const Shaders*>(IBaseComputePipeline::shaders()); }

	//--				

private:
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
