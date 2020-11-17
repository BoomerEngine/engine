/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importer.h"
#include "importInterface.h"
#include "importInterfaceImpl.h"
#include "importSourceAssetRepository.h"

#include "base/resource/include/resourceMetadata.h"

namespace base
{
    namespace res
    {

        //--

        LocalImporterInterface::LocalImporterInterface(SourceAssetRepository* assetRepository, const IImportDepotChecker* depot, const IResource* originalData, const StringBuf& importPath, const StringBuf& depotPath, const ResourceMountPoint& depotMountPoint, IProgressTracker* externalProgressTracker, const ResourceConfigurationPtr& configuration)
            : m_originalData(originalData)
            , m_importPath(importPath)
            , m_depotPath(depotPath)
            , m_depotMountPoint(depotMountPoint)
            , m_externalProgressTracker(externalProgressTracker)
            , m_assetRepository(assetRepository)
            , m_configuration(configuration)
            , m_depotChecker(depot)
        {
            ASSERT(configuration != nullptr)
        }

        LocalImporterInterface::~LocalImporterInterface()
        {}

        const IResource* LocalImporterInterface::existingData() const
        {
            return m_originalData;
        }

        const StringBuf& LocalImporterInterface::queryResourcePath() const
        {
            return m_depotPath;
        }

        const ResourceMountPoint& LocalImporterInterface::queryResourceMountPoint() const
        {
            return m_depotMountPoint;
        }

        const StringBuf& LocalImporterInterface::queryImportPath() const
        {
            return m_importPath;
        }

        const ResourceConfiguration* LocalImporterInterface::queryConfigrationTypeless() const
        {
            return m_configuration;
        }

        Buffer LocalImporterInterface::loadSourceFileContent(StringView assetImportPath) const
        {
            io::TimeStamp timestamp;
            ImportFileFingerprint fingerprint;
            if (auto ret = m_assetRepository->loadSourceFileContent(assetImportPath, timestamp, fingerprint))
            {
                const_cast<LocalImporterInterface*>(this)->reportImportDependency(assetImportPath, timestamp, fingerprint);
                return ret;
            }

            return Buffer();
        }

        SourceAssetPtr LocalImporterInterface::loadSourceAsset(StringView assetImportPath) const
        {
            io::TimeStamp timestamp;
            ImportFileFingerprint fingerprint;
            if (auto ret = m_assetRepository->loadSourceAsset(assetImportPath, timestamp, fingerprint))
            {
                const_cast<LocalImporterInterface*>(this)->reportImportDependency(assetImportPath, timestamp, fingerprint);
                return ret;
            }

            return nullptr;
        }

        void LocalImporterInterface::reportImportDependency(StringView assetImportPath, const io::TimeStamp& timestamp, const ImportFileFingerprint& fingerprint)
        {
            auto lock = CreateLock(m_importDependenciesLock);

            const auto assetKey = StringBuf(assetImportPath).toLower();

            if (m_importDependenciesSet.insert(assetKey))
            {
                auto& entry = m_importDependencies.emplaceBack();
                entry.assetPath = StringBuf(assetImportPath);
                entry.fingerprint = fingerprint;
                TRACE_INFO("Reported '{}' as import dependency, last modified at {}, fingerprint: {}", assetImportPath, timestamp, fingerprint);
            }
        }

        //--

        bool LocalImporterInterface::findSourceFile(StringView assetImportPath, StringView inputPath, StringBuf& outImportPath, uint32_t maxScanDepth /*= 2*/) const
        {
            return ScanRelativePaths(assetImportPath, inputPath, maxScanDepth, outImportPath, [this](StringView testPath)
                {
                    return m_assetRepository->fileExists(testPath);
                });
        }

        void LocalImporterInterface::followupImport(StringView assetImportPath, StringView depotPath, const ResourceConfiguration* config)
        {
            if (assetImportPath && depotPath)
            {
                const auto depotKey = StringBuf(depotPath).toLower();

                if (m_followupImportsSet.insert(depotKey))
                {
                    auto& entry = m_followupImports.emplaceBack();
                    entry.assetPath = StringBuf(assetImportPath);
                    entry.depotPath = StringBuf(depotPath);
                    entry.config = AddRef(config);
                }
                else
                {
                    TRACE_WARNING("Followup import '{}' already specified", depotPath);
                }
            }
        }

        //--

        bool LocalImporterInterface::findDepotFile(StringView depotReferencePath, StringView depotSearchPath, StringView searchFileName, StringBuf& outDepotPath, uint32_t maxScanDepth) const
        {
            StringBuf depotFinalSearchPath;
            if (!ApplyRelativePath(depotReferencePath, depotSearchPath, depotFinalSearchPath))
                return false;

            return m_depotChecker->depotFindFile(depotFinalSearchPath, searchFileName, maxScanDepth, outDepotPath);
        }

        //--

        MetadataPtr LocalImporterInterface::buildMetadata() const
        {
            auto ret = RefNew<Metadata>();

            ret->importDependencies.reserve(m_importDependencies.size());
            for (const auto& dep : m_importDependencies)
            {
                auto& entry = ret->importDependencies.emplaceBack();
                entry.importPath = dep.assetPath;
                entry.timestamp = dep.timestamp;
                entry.crc = dep.fingerprint.rawValue();
            }

            ret->importFollowups.reserve(m_followupImports.size());
            for (const auto& info : m_followupImports)
            {
                auto& entry = ret->importFollowups.emplaceBack();
                entry.sourceImportPath = info.assetPath;
                entry.depotPath = info.depotPath;
                entry.configuration = CloneObject(info.config, ret);
            }

            return ret;
        }

        //--

        bool LocalImporterInterface::checkCancelation() const
        {
            if (m_externalProgressTracker)
                return m_externalProgressTracker->checkCancelation();
            return false;
        }

        void LocalImporterInterface::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
        {
            if (m_externalProgressTracker)
                m_externalProgressTracker->reportProgress(currentCount, totalCount, text);
        }

        //--

    } // res
} // base
