/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\cooking #]
***/

#pragma once

#include "base/containers/include/refCounted.h"

namespace base
{
    namespace res
    {

        //--

        /// a magical class that can load a resource from raw source data
        /// NOTE: file loaders are for resources that are simple enough to load directly, without lengthy baking
        class BASE_RESOURCE_API IResourceCooker : public base::IReferencable
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IResourceCooker);

        public:
            IResourceCooker();
            virtual ~IResourceCooker();

            /// use the data at your disposal that you can query via the cooker interface
            virtual ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const = 0;
        };

        //---

    } // res
} // base