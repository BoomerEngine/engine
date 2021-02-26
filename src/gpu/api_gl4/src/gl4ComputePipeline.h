/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gpu/api_common/include/apiComputePipeline.h"

#include "gl4Thread.h"
#include "gl4Shaders.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

///---

class ComputePipeline : public IBaseComputePipeline
{
public:
	ComputePipeline(Thread* owner, const Shaders* shaders);
	virtual ~ComputePipeline();

	//--

	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
	INLINE Shaders* shaders() const { return const_cast<Shaders*>(static_cast<const Shaders*>(IBaseComputePipeline::shaders())); }

	//--				

	bool apply(GLuint& glActiveProgram);

	//--

private:
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
