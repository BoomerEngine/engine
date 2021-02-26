/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiDescriptorLayout.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//--

DescriptorBindingLayout::DescriptorBindingLayout(Thread* owner, const Array<ShaderDescriptorMetadata>& descriptors)
	: IBaseDescriptorBindingLayout(descriptors)
{}

DescriptorBindingLayout::~DescriptorBindingLayout()
{}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
