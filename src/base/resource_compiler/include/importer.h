/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once
#include "base/containers/include/hashMap.h"

namespace base
{
    namespace res
    {
        //--
        
        /// single import job
        struct BASE_RESOURCE_COMPILER_API ImportJobInfo
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(ImportJobInfo)

            StringBuf assetFilePath;
            StringBuf depotFilePath;

            // NOTE: configuration objects must be free-objects (not parented to existing hierarchy)
            mutable ResourceConfigurationPtr externalConfig; // follow-up import configuration 
            ResourceConfigurationPtr userConfig; // user given configuration

            bool followImports = true; // any resource import can spawn other imports
            bool forceImport = false;
        };

        //--

        /// helper class to check if given depot file already exists
        class BASE_RESOURCE_COMPILER_API IImportDepotChecker : public NoCopy
        {
        public:
            virtual ~IImportDepotChecker();

            // check if file exists
            virtual bool depotFileExists(StringView depotPath) const = 0;

            // given a depot root path and file name find existing depot file
            virtual bool depotFindFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const = 0;
        };

        //--

        /// helper class that can import resources from source files
        class BASE_RESOURCE_COMPILER_API Importer : public NoCopy
        {
        public:
            Importer(SourceAssetRepository* assets, const IImportDepotChecker* depotChecker);
            ~Importer();

            //--

            /// check if given resource is up to date
            /// NOTE: we need the metadata loaded to check this
            ImportStatus checkStatus(const StringBuf& depotPath, const Metadata& metadata, const ResourceConfigurationPtr& newUserConfiguration = nullptr, IProgressTracker* progress = nullptr) const;

            /// import single resource, produces imported resource (with meta data)
            ImportStatus importResource(const ImportJobInfo& info, const IResource* existingData, ResourcePtr& outImportedResource, IProgressTracker* progress=nullptr) const;

            //--

        private:
            struct ImportableClass
            {
                SpecificClassType<IResource> targetResourceClass = nullptr;
                SpecificClassType<IResourceImporter> importerClass = nullptr;
            };

            HashMap<StringBuf, Array<ImportableClass>> m_importableClassesPerExtension; // list of importable formats (per extension)

            void buildClassMap();

            bool findBestImporter(StringView assetFilePath, SpecificClassType<IResource>, SpecificClassType<IResourceImporter>& outImporterClass) const;

            ResourceConfigurationPtr compileFinalImportConfiguration(const StringBuf& depotPath, const Metadata& metadata, const ResourceConfigurationPtr& newUserConfiguration) const;

            //--

            SourceAssetRepository* m_assets;

            const IImportDepotChecker* m_depotChecker;
        };

        //--

    } // res
} // base