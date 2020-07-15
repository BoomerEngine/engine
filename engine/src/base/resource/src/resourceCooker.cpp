/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\cooking #]
***/

#include "build.h"
#include "resource.h"
#include "resourceCooker.h"
#include "resourceTags.h"

namespace base
{
    namespace res
    {
        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceCooker);
        RTTI_METADATA(ResourceCookerVersionMetadata).version(0);
        RTTI_END_TYPE();

        IResourceCooker::IResourceCooker()
        {}

        IResourceCooker::~IResourceCooker()
        {}
        
        //--

    } // res
} // base