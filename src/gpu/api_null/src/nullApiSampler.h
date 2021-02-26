/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gpu/api_common/include/apiSampler.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

///---

class Sampler : public IBaseSampler
{
public:
	Sampler(Thread* owner, const SamplerState& setup);
	virtual ~Sampler();

	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
