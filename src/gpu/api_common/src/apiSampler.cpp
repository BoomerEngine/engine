/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiSampler.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

IBaseSampler::IBaseSampler(IBaseThread* owner, const SamplerState& setup)
	: IBaseObject(owner, ObjectType::Sampler)
	, m_state(setup)
{}

IBaseSampler::~IBaseSampler()
{}

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
