/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiThread.h"
#include "nullApiSampler.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//--

Sampler::Sampler(Thread* owner, const SamplerState& setup)
	: IBaseSampler(owner, setup)
{}

Sampler::~Sampler()
{}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
