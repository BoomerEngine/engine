/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
***/

#include "build.h"
#include "nullApiComputePipeline.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//--

			ComputePipeline::ComputePipeline(Thread* owner, const Shaders* shaders)
				: IBaseComputePipeline(owner, shaders)
			{}

			ComputePipeline::~ComputePipeline()
			{}

			//--

		} // nul
    } // gl4
} // rendering
