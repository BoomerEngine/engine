/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once
#include "importFileFingerprint.h"

namespace base
{
    namespace res
    {

        //--

        class SourceAssetRepository;

        //--

        /// asset import interface - implementation
        class BASE_RESOURCE_COMPILER_API LocalImporterInterface : public IResourceImporterInterface
        {
        public:
            LocalImporterInterface(SourceAssetRepository* assetRepository, const IResource* originalData, const StringBuf& importPath, const ResourcePath& depotPath, const ResourceMountPoint& depotMountPoint, IProgressTracker* externalProgressTracker, const ResourceConfigurationPtr& importConfiguration);
            virtual ~LocalImporterInterface();

            /// IResourceImporterInterface
            virtual const IResource* existingData() const override final;
            virtual const ResourcePath& queryResourcePath() const override final;
            virtual const ResourceMountPoint& queryResourceMountPoint() const  override final;
            virtual const StringBuf& queryImportPath() const  override final;
            virtual const ResourceConfiguration* queryConfigrationTypeless() const  override final;
            virtual Buffer loadSourceFileContent(StringView<char> assetImportPath) const override final;
            virtual SourceAssetPtr loadSourceAsset(StringView<char> assetImportPath) const override final;
            virtual bool findSourceFile(StringView<char> assetImportPath, StringView<char> inputPath, StringBuf& outImportPath, uint32_t maxScanDepth = 2) const  override final;
            virtual void followupImport(StringView<char> assetImportPath, StringView<char> depotPath, const ResourceConfiguration* config = nullptr)  override final;

            // IProgressTracker
            virtual bool checkCancelation() const override final;
            virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text) override final;

            // build import metadata from all gathered stuff
            MetadataPtr buildMetadata() const;

        private:
            const IResource* m_originalData;

            StringBuf m_importPath;
            ResourcePath m_depotPath;
            ResourceMountPoint m_depotMountPoint;
            
            IProgressTracker* m_externalProgressTracker = nullptr;

            SourceAssetRepository* m_assetRepository = nullptr;

            ResourceConfigurationPtr m_configuration; // merged import configuration

            mutable Array<ResourceConfigurationPtr> m_tempConfigurations;
            SpinLock m_tempConfigurationsLock;

            //--

            struct FollowupImport
            {
                StringBuf assetPath;
                StringBuf depotPath;
                ResourceConfigurationPtr config;
            };

            Array<FollowupImport> m_followupImports;
            HashSet<StringBuf> m_followupImportsSet;

            //--

            struct ImportDependencies
            {
                StringBuf assetPath;
                ImportFileFingerprint fingerprint;
            };

            SpinLock m_importDependenciesLock;
            Array<ImportDependencies> m_importDependencies;
            HashSet<StringBuf> m_importDependenciesSet;

            void reportImportDependency(StringView<char> assetImportPath, const ImportFileFingerprint& fingerprint);
        };

        //--

    } // res
} // base