/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/api_common/include/apiObjectCache.h"
#include "dx11Thread.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::dx11)

//---

class ObjectCache : public IBaseObjectCache
{
public:
	ObjectCache(Thread* owner);
	virtual ~ObjectCache();

	//--

	INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObjectCache::owner()); }

	//--

	virtual IBaseVertexBindingLayout* createOptimalVertexBindingLayout(const Array<ShaderVertexStreamMetadata>& streams) override;
	virtual IBaseDescriptorBindingLayout* createOptimalDescriptorBindingLayout(const Array<ShaderDescriptorMetadata>& descriptors, const Array<ShaderStaticSamplerMetadata>& staticSamplers) override;
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api::dx11)
