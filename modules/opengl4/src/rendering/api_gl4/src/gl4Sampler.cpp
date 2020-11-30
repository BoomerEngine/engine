/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4Sampler.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			Sampler::Sampler(Thread* owner, const SamplerState& setup)
				: IBaseSampler(owner, setup)
			{}

			Sampler::~Sampler()
			{}

			//--

		} // gl4
    } // gl4
} // rendering
