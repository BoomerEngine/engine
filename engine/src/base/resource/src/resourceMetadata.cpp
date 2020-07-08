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

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceConfiguration);
        RTTI_END_TYPE();

        IResourceConfiguration::~IResourceConfiguration()
        {}

        //--

        RTTI_BEGIN_TYPE_STRUCT(SourceDependency);
            RTTI_PROPERTY(sourcePath);
            RTTI_PROPERTY(timestamp);
            RTTI_PROPERTY(size);
            RTTI_PROPERTY(crc);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_STRUCT(ImportDependency);
            RTTI_PROPERTY(importPath);
            RTTI_PROPERTY(crc);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_STRUCT(ImportFollowup);
            RTTI_PROPERTY(depotPath);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(Metadata);
            RTTI_PROPERTY(sourceDependencies);
            RTTI_PROPERTY(importDependencies);
            RTTI_PROPERTY(importFollowups);
            RTTI_PROPERTY(cookerClassVersion);
            RTTI_PROPERTY(resourceClassVersion);
            RTTI_PROPERTY(importConfigurations);
            RTTI_PROPERTY(cookerClass);
            RTTI_PROPERTY(internalRevision);
        RTTI_END_TYPE();

        //--

    } // res
} // base

