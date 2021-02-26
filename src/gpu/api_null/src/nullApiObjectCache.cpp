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

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//---

ObjectCache::ObjectCache(Thread* owner)
	: IBaseObjectCache(owner)
{}

ObjectCache::~ObjectCache()
{}

IBaseVertexBindingLayout* ObjectCache::createOptimalVertexBindingLayout(const Array<ShaderVertexStreamMetadata>& streams)
{
	return new VertexBindingLayout(owner(), streams);
}

IBaseDescriptorBindingLayout* ObjectCache::createOptimalDescriptorBindingLayout(const Array<ShaderDescriptorMetadata>& descriptors, const Array<ShaderStaticSamplerMetadata>& staticSamplers)
{
	return new DescriptorBindingLayout(owner(), descriptors);
}

//---

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
