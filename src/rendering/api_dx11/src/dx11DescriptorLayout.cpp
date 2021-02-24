/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "dx11DescriptorLayout.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::dx11)

//--

DescriptorBindingLayout::DescriptorBindingLayout(Thread* owner, const base::Array<ShaderDescriptorMetadata>& descriptors)
	: IBaseDescriptorBindingLayout(descriptors)
{}

DescriptorBindingLayout::~DescriptorBindingLayout()
{}

//--

END_BOOMER_NAMESPACE(rendering::api::dx11)