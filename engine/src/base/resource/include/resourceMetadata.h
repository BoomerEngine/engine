/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\metadata #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/reflection/include/variantTable.h"

namespace base
{
    namespace res
    {
        class IResourceCooker;

        ///---

        /// persistent resource "configuration" - carried over from one reimport to other reimport (all other data is lost)
        class BASE_RESOURCE_API IResourceConfiguration : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IResourceConfiguration, IObject);

        public:
            virtual ~IResourceConfiguration();

            /// TODO: configuration "hash" ?
        };

        ///---

        /// resource source dependency information
        /// contains information of a file used to import given resource
        struct BASE_RESOURCE_API SourceDependency
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SourceDependency);

        public:
            StringBuf sourcePath; // depot file used during resource cooking
            uint64_t size = 0; // size of the file, larger files may skip CRC check and just assume that are not equal
            uint64_t timestamp = 0; // timestamp of the file at the time of cooking, if still valid files are assumed to be ok
            uint64_t crc = 0; // CRC of the data - set if known
        };

        ///---

        /// resource EXTERNAL (asset) dependency information
        struct BASE_RESOURCE_API ImportDependency
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(ImportDependency);

        public:
            StringBuf importPath; // full absolute path (saved as UTF-8) to source import path
            uint64_t crc = 0; // CRC of the source import file
        };

        ///---

        /// List of files that have to be imported as well for this resource to work fine
        /// usually mesh requires materials, materials require textures
        /// This list is used so even when the original file was up to date we will know what other files we need to check
        struct BASE_RESOURCE_API ImportFollowup
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(ImportFollowup);

            StringBuf depotPath; // where to import, extension encodes type
            StringBuf sourceImportPath; // from where to import
            Array<ResourceConfigurationPtr> configuration; // explicit configuration entries, not used if resource already exists
        };

        ///---

        /// metadata of the resource, saved in .meta file alongside the .cache file
        /// NOTE: metadata can contain information that can help to detect that file does not require full cooking
        class BASE_RESOURCE_API Metadata : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Metadata, IObject);

        public:
            // cooker version that was used to cook this resource, if this does not match we need to recook
            uint32_t cookerClassVersion = 0;

            // resource data version, if this does not match we need to recook as well
            uint32_t resourceClassVersion = 0;

            // internal revision count - each time file is reimported this number is incremented
            uint32_t internalRevision = 0;

            // cooker class that was used to import the resource (if any)
            ClassType cookerClass;

            // source dependencies of the file
            // dependency 0 is the root file
            Array<SourceDependency> sourceDependencies;

            // import dependencies of the file (for imported files only)
            Array<ImportDependency> importDependencies;

            // other assets that were imported due to import of this file
            Array<ImportFollowup> importFollowups;

            // import configurations
            Array<ResourceConfigurationPtr> importConfigurations;

            // additional values that can be read/written by the cooker
            // this is a good place to store CRC that can detect 
            //VariantTable blackboard;
        };

        ///---

    } // res
} // base