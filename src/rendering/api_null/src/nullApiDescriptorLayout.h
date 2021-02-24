/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiObjectCache.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::nul)

//--

class DescriptorBindingLayout : public IBaseDescriptorBindingLayout
{
public:
	DescriptorBindingLayout(Thread* owner, const base::Array<ShaderDescriptorMetadata>& descriptors);
	virtual ~DescriptorBindingLayout();
};

//--

END_BOOMER_NAMESPACE(rendering::api::nul)