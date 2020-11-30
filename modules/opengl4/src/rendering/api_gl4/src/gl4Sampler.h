/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiSampler.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{
			///---

			class Sampler : public IBaseSampler
			{
			public:
				Sampler(Thread* owner, const SamplerState& setup);
				virtual ~Sampler();

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
			};

			//--

		} // gl4
    } // api
} // rendering

