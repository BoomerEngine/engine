/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "dx11ObjectCache.h"
#include "dx11DescriptorLayout.h"
#include "dx11VertexLayout.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::dx11)

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

END_BOOMER_NAMESPACE_EX(gpu::api::dx11)
