/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiObjectCache.h"
#include "gl4Thread.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
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

				virtual IBaseVertexBindingLayout* createOptimalVertexBindingLayout(const base::Array<ShaderVertexStreamMetadata>& streams) override;
				virtual IBaseDescriptorBindingLayout* createOptimalDescriptorBindingLayout(const base::Array<ShaderDescriptorMetadata>& descriptors) override;
			};

			//---

		} // gl4
    } // api
} // rendering