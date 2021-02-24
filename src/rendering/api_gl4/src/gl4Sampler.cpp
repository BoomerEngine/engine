/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4ObjectCache.h"
#include "gl4Sampler.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::gl4)

//--

Sampler::Sampler(Thread* owner, const SamplerState& setup)
	: IBaseSampler(owner, setup)
{}

Sampler::~Sampler()
{}

GLuint Sampler::object()
{
	if (m_glSampler)
		return m_glSampler;

	m_glSampler = owner()->objectCache()->createSampler(state());
	return m_glSampler;
}

//--

END_BOOMER_NAMESPACE(rendering::api::gl4)