/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "dx11DescriptorLayout.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			//--

			DescriptorBindingLayout::DescriptorBindingLayout(Thread* owner, const base::Array<ShaderDescriptorMetadata>& descriptors)
				: IBaseDescriptorBindingLayout(descriptors)
			{}

			DescriptorBindingLayout::~DescriptorBindingLayout()
			{}

			//--

		} // dx11
    } // gl4
} // rendering
