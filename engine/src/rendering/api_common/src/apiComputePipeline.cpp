/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiComputePipeline.h"
#include "apiShaders.h"

namespace rendering
{
    namespace api
    {
		//--

		IBaseComputePipeline::IBaseComputePipeline(IBaseThread* owner, const IBaseShaders* shaders)
			: IBaseObject(owner, ObjectType::ComputePipelineObject)
			, m_shaders(shaders)
		{
			m_key = shaders->key();
		}

		IBaseComputePipeline::~IBaseComputePipeline()
		{}

		//--

    } // api
} // rendering
