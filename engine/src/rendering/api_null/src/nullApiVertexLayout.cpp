/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiThread.h"
#include "nullApiObjectCache.h"
#include "nullApiVertexLayout.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//--

			VertexBindingLayout::VertexBindingLayout(Thread* owner, base::Array<IBaseVertexBindingLayout::BindingInfo>&& elements)
				: IBaseVertexBindingLayout(std::move(elements))
			{}

			VertexBindingLayout::~VertexBindingLayout()
			{}

			//--

		} // nul
    } // gl4
} // rendering
