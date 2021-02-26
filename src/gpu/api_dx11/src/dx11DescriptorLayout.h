/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gpu/api_common/include/apiObjectCache.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::dx11)

//--

class DescriptorBindingLayout : public IBaseDescriptorBindingLayout
{
public:
	DescriptorBindingLayout(Thread* owner, const Array<ShaderDescriptorMetadata>& descriptors);
	virtual ~DescriptorBindingLayout();
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::dx11)
