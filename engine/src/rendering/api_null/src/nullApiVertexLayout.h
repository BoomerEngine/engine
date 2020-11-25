/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiObjectCache.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{
			//--

			class VertexBindingLayout : public IBaseVertexBindingLayout
			{
			public:
				VertexBindingLayout(Thread* owner, base::Array<IBaseVertexBindingLayout::BindingInfo>&& elements);
				virtual ~VertexBindingLayout();
			};

			//--

		} // nul
    } // api
} // rendering

