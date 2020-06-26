/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\metadata #]
***/

#pragma once

#include "base/resources/include/resource.h"
#include "base/reflection/include/variantTable.h"

namespace base
{
    namespace res
    {
        class IResourceCooker;

        /// resource source dependency information
        /// contains information of a file used to import given resource
        struct BASE_RESOURCES_API SourceDependency
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SourceDependency);

        public:
            StringBuf sourcePath; // depot import path of raw uncooked file (NOT a resource path and NOT an absolute path)
            uint64_t size = 0; // size of the file, larger files may skip CRC check and just assume that are not equal
            uint64_t timestamp = 0; // timestamp of the file at the time of cooking
            uint64_t crc = 0; // CRC of the data - set if known
        };

        ///---

        /// metadata of the resource, saved in .meta file alongside the .cache file
        /// NOTE: metadata can contain information that can help to detect that file does not require full cooking
        class BASE_RESOURCES_API Metadata : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Metadata, IObject);

        public:
            // cooker version that was used to cook this resource, if this does not match we need to recook
            uint32_t cookerClassVersion = 0;

            // resource data version, if this does not match we need to recook as well
            uint32_t resourceClassVersion = 0;

            // cooker class that was used to import the resource (if any)
            SpecificClassType<IResourceCooker> cookerClass;

            // source dependencies of the file
            // dependency 0 is the root file
            typedef Array<SourceDependency> TSourceDependencies;
            TSourceDependencies sourceDependencies;

            // additional values that can be read/written by the cooker
            // this is a good place to store CRC that can detect 
            //VariantTable blackboard;
        };

    } // res
} // base