/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "dx11Thread.h"
#include "dx11ObjectCache.h"
#include "dx11VertexLayout.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			//--

			VertexBindingLayout::VertexBindingLayout(Thread* owner, const base::Array<ShaderVertexStreamMetadata>& streams)
				: IBaseVertexBindingLayout(owner->objectCache(), streams)
			{}

			VertexBindingLayout::~VertexBindingLayout()
			{}

			//--

		} // dx11
    } // gl4
} // rendering
