/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
***/

#pragma once

#include "rendering/api_common/include/apiComputePipeline.h"

#include "dx11Thread.h"
#include "dx11Shaders.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{
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

		} // dx11
    } // api
} // rendering

