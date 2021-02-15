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

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//--

			Sampler::Sampler(Thread* owner, const SamplerState& setup)
				: IBaseSampler(owner, setup)
			{}

			Sampler::~Sampler()
			{}

			//--

		} // nul
    } // gl4
} // rendering
