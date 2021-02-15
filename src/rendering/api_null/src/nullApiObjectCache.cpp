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

			IBaseVertexBindingLayout* ObjectCache::createOptimalVertexBindingLayout(const base::Array<ShaderVertexStreamMetadata>& streams)
			{
				return new VertexBindingLayout(owner(), streams);
			}

			IBaseDescriptorBindingLayout* ObjectCache::createOptimalDescriptorBindingLayout(const base::Array<ShaderDescriptorMetadata>& descriptors, const base::Array<ShaderStaticSamplerMetadata>& staticSamplers)
			{
				return new DescriptorBindingLayout(owner(), descriptors);
			}

			//---

		} // nul
	} // api
} // rendering
