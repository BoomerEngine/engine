/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "nullApiObjectCache.h"
#include "nullApiDescriptorLayout.h"
#include "nullApiVertexLayout.h"

namespace rendering
{
	namespace api
	{
		namespace nul
		{

			//---

			ObjectCache::ObjectCache(Thread* owner)
				: IBaseObjectCache(owner)
			{}

			ObjectCache::~ObjectCache()
			{}

			IBaseVertexBindingLayout* ObjectCache::createOptimalVertexBindingLayout(base::Array<IBaseVertexBindingLayout::BindingInfo>&& elements)
			{
				return new VertexBindingLayout(owner(), std::move(elements));
			}

			IBaseDescriptorBindingLayout* ObjectCache::createOptimalDescriptorBindingLayout(base::Array<DescriptorBindingElement>&& elements)
			{
				return new DescriptorBindingLayout(owner(), std::move(elements));
			}

			//---

		} // nul
	} // api
} // rendering
