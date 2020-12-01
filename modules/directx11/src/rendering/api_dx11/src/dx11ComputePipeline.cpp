/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
***/

#include "build.h"
#include "dx11ComputePipeline.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			//--

			ComputePipeline::ComputePipeline(Thread* owner, const Shaders* shaders)
				: IBaseComputePipeline(owner, shaders)
			{}

			ComputePipeline::~ComputePipeline()
			{}

			//--

		} // dx11
    } // gl4
} // rendering
