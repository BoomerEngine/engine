/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiComputePipeline.h"

#include "gl4Thread.h"
#include "gl4Shaders.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
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

				bool apply();

				//--

			private:
			};

			//--

		} // gl4
    } // api
} // rendering

