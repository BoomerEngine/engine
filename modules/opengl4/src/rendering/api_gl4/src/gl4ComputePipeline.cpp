/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4ComputePipeline.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			ComputePipeline::ComputePipeline(Thread* owner, const Shaders* shaders)
				: IBaseComputePipeline(owner, shaders)
			{}

			ComputePipeline::~ComputePipeline()
			{}

			bool ComputePipeline::apply()
			{
				return false;
			}

			//--

		} // gl4
    } // gl4
} // rendering
