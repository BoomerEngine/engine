/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once
#include "importFileFingerprint.h"
#include "base/io/include/timestamp.h"

namespace base
{
    namespace res
    {

        //--

        class SourceAssetRepository;
        class IImportDepotChecker;

        //--

        /// asset import interface - implementation
        class BASE_RESOURCE_COMPILER_API LocalImporterInterface : public IResourceImporterInterface
        {
        public:
            LocalImporterInterface(SourceAssetRepository* assetRepository, const IImportDepotChecker* depot, const IResource* originalData, const StringBuf& importPath, const StringBuf& depotPath, IProgressTracker* externalProgressTracker, const ResourceConfigurationPtr& importConfiguration);
            virtual ~LocalImporterInterface();

            /// IResourceImporterInterface
            virtual const IResource* existingData() const override final;
            virtual const StringBuf& queryResourcePath() const override final;
            virtual const StringBuf& queryImportPath() const  override final;
            virtual const ResourceConfiguration* queryConfigrationTypeless() const  override final;
            virtual Buffer loadSourceFileContent(StringView assetImportPath) const override final;
            virtual SourceAssetPtr loadSourceAsset(StringView assetImportPath) const override final;
            virtual bool findSourceFile(StringView assetImportPath, StringView inputPath, StringBuf& outImportPath, uint32_t maxScanDepth = 2) const override final;
            virtual bool findDepotFile(StringView depotReferencePath, StringView depotSearchPath, StringView searchFileName, StringBuf& outDepotPath, uint32_t maxScanDepth = 2) const override final;
            virtual bool checkDepotFile(StringView depotPath) const override final;
            virtual bool checkSourceFile(StringView assetImportPath) const override final;
            virtual void followupImport(StringView assetImportPath, StringView depotPath, const ResourceConfiguration* config = nullptr)  override final;

            // IProgressTracker
            virtual bool checkCancelation() const override final;
            virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final;

            // build import metadata from all gathered stuff
            MetadataPtr buildMetadata() const;

        private:
            const IResource* m_originalData;

            const IImportDepotChecker* m_depotChecker;

            StringBuf m_importPath;
            StringBuf m_depotPath;
            
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
                io::TimeStamp timestamp;
                ImportFileFingerprint fingerprint;
            };

            SpinLock m_importDependenciesLock;
            Array<ImportDependencies> m_importDependencies;
            HashSet<StringBuf> m_importDependenciesSet;

            void reportImportDependency(StringView assetImportPath, const io::TimeStamp& timestamp, const ImportFileFingerprint& fingerprint);
        };

        //--

    } // res
} // base