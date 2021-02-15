/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "dx11Thread.h"
#include "dx11Sampler.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			//--

			Sampler::Sampler(Thread* owner, const SamplerState& setup)
				: IBaseSampler(owner, setup)
			{}

			Sampler::~Sampler()
			{}

			//--

		} // dx11
    } // gl4
} // rendering
