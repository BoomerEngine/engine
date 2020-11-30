/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4ObjectCache.h"
#include "gl4VertexLayout.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			VertexBindingLayout::VertexBindingLayout(Thread* owner, const base::Array<ShaderVertexStreamMetadata>& streams)
				: IBaseVertexBindingLayout(streams)
			{}

			VertexBindingLayout::~VertexBindingLayout()
			{}

			//--

		} // gl4
    } // gl4
} // rendering
