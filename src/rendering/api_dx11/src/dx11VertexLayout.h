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
		namespace dx11
		{
			//--

			class VertexBindingLayout : public IBaseVertexBindingLayout
			{
			public:
				VertexBindingLayout(Thread* owner, const base::Array<ShaderVertexStreamMetadata>& streams);
				virtual ~VertexBindingLayout();
			};

			//--

		} // dx11
    } // api
} // rendering

