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

BEGIN_BOOMER_NAMESPACE(rendering::api::nul)

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
	virtual IBaseDescriptorBindingLayout* createOptimalDescriptorBindingLayout(const base::Array<ShaderDescriptorMetadata>& descriptors, const base::Array<ShaderStaticSamplerMetadata>& staticSamplers) override;
};

//---

END_BOOMER_NAMESPACE(rendering::api::nul)