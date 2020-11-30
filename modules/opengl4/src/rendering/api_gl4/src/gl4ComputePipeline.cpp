/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
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

			//--

		} // gl4
    } // gl4
} // rendering
