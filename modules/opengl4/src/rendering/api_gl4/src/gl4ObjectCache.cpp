/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "gl4ObjectCache.h"
#include "gl4DescriptorLayout.h"
#include "gl4VertexLayout.h"

namespace rendering
{
	namespace api
	{
		namespace gl4
		{

			//---

			ObjectCache::ObjectCache(Thread* owner)
				: IBaseObjectCache(owner)
			{}

			ObjectCache::~ObjectCache()
			{}

			IBaseVertexBindingLayout* ObjectCache::createOptimalVertexBindingLayout(const base::Array<ShaderVertexStreamMetadata>& streams)
			{
				return new VertexBindingLayout(owner(), streams);
			}

			IBaseDescriptorBindingLayout* ObjectCache::createOptimalDescriptorBindingLayout(const base::Array<ShaderDescriptorMetadata>& descriptors)
			{
				return new DescriptorBindingLayout(owner(), descriptors);
			}

			//---

		} // gl4
	} // api
} // rendering
