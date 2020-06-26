/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\metadata #]
***/

#include "build.h"
#include "resourceMetadata.h"
#include "resource.h"

namespace base
{
    namespace res
    {

        //--

        RTTI_BEGIN_TYPE_STRUCT(SourceDependency);
            RTTI_PROPERTY(sourcePath);
            RTTI_PROPERTY(timestamp);
            RTTI_PROPERTY(size);
            RTTI_PROPERTY(crc);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(Metadata);
            RTTI_PROPERTY(sourceDependencies);
            RTTI_PROPERTY(cookerClassVersion);
            RTTI_PROPERTY(resourceClassVersion);
            RTTI_PROPERTY(cookerClass);
        RTTI_END_TYPE();

        //--

    } // res
} // base

