/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiObjectCache.h"
#include "nullApiThread.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{
			//---

			class ObjectCache : public IBaseObjectCache
			{
			public:
				ObjectCache(Thread* owner);
				virtual ~ObjectCache();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObjectCache::owner()); }

				//--

				virtual IBaseVertexBindingLayout* createOptimalVertexBindingLayout(base::Array<IBaseVertexBindingLayout::BindingInfo>&& elements) override final;
				virtual IBaseDescriptorBindingLayout* createOptimalDescriptorBindingLayout(base::Array<DescriptorBindingElement>&& elements) override final;
			};

			//---

		} // nul
    } // api
} // rendering