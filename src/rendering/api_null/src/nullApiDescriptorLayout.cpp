/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiDescriptorLayout.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//--

			DescriptorBindingLayout::DescriptorBindingLayout(Thread* owner, const base::Array<ShaderDescriptorMetadata>& descriptors)
				: IBaseDescriptorBindingLayout(descriptors)
			{}

			DescriptorBindingLayout::~DescriptorBindingLayout()
			{}

			//--

		} // nul
    } // gl4
} // rendering
