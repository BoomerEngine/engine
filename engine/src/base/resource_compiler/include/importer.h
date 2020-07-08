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

            Array<ResourceConfigurationPtr> config;
        };

        //--

        /// helper class that can import resources from source files
        class BASE_RESOURCE_COMPILER_API Importer : public NoCopy
        {
        public:
            Importer(SourceAssetRepository* assets);
            ~Importer();

            //--

            /// check if given resource is up to date
            /// NOTE: we need the metadata loaded to check this
            ImportStatus checkStatus(const Metadata& metadata) const;

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

            bool findBestImporter(StringView<char> assetFilePath, SpecificClassType<IResource>, SpecificClassType<IResourceImporter>& outImporterClass) const;

            //--

            SourceAssetRepository* m_assets;
        };

        //--

    } // res
} // base