/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importInterface.h"
#include "importInterfaceImpl.h"
#include "base/resource/include/resourceMetadata.h"
#include "importSourceAssetRepository.h"

namespace base
{
    namespace res
    {

        //--

        LocalImporterInterface::LocalImporterInterface(SourceAssetRepository* assetRepository, const IResource* originalData, const StringBuf& importPath, const ResourcePath& depotPath, const ResourceMountPoint& depotMountPoint, IProgressTracker* externalProgressTracker, const ResourceConfigurationPtr& configuration)
            : m_originalData(originalData)
            , m_importPath(importPath)
            , m_depotPath(depotPath)
            , m_depotMountPoint(depotMountPoint)
            , m_externalProgressTracker(externalProgressTracker)
            , m_assetRepository(assetRepository)
            , m_configuration(configuration)
        {
            ASSERT(configuration != nullptr)
        }

        LocalImporterInterface::~LocalImporterInterface()
        {}

        const IResource* LocalImporterInterface::existingData() const
        {
            return m_originalData;
        }

        const ResourcePath& LocalImporterInterface::queryResourcePath() const
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

        Buffer LocalImporterInterface::loadSourceFileContent(StringView<char> assetImportPath) const
        {
            ImportFileFingerprint fingerprint;
            if (auto ret = m_assetRepository->loadSourceFileContent(assetImportPath, fingerprint))
            {
                const_cast<LocalImporterInterface*>(this)->reportImportDependency(assetImportPath, fingerprint);
                return ret;
            }

            return Buffer();
        }

        SourceAssetPtr LocalImporterInterface::loadSourceAsset(StringView<char> assetImportPath) const
        {
            ImportFileFingerprint fingerprint;
            if (auto ret = m_assetRepository->loadSourceAsset(assetImportPath, fingerprint))
            {
                const_cast<LocalImporterInterface*>(this)->reportImportDependency(assetImportPath, fingerprint);
                return ret;
            }

            return nullptr;
        }

        void LocalImporterInterface::reportImportDependency(StringView<char> assetImportPath, const ImportFileFingerprint& fingerprint)
        {
            auto lock = CreateLock(m_importDependenciesLock);

            const auto assetKey = StringBuf(assetImportPath).toLower();

            if (m_importDependenciesSet.insert(assetKey))
            {
                auto& entry = m_importDependencies.emplaceBack();
                entry.assetPath = StringBuf(assetImportPath);
                entry.fingerprint = fingerprint;
                TRACE_INFO("Reported '{}' as import dependency", assetImportPath);
            }
        }

        //--

        bool LocalImporterInterface::findSourceFile(StringView<char> assetImportPath, StringView<char> inputPath, StringBuf& outImportPath, uint32_t maxScanDepth /*= 2*/) const
        {
            // slice the input path
            InplaceArray<StringView<char>, 20> inputParts;
            inputPath.slice("\\/", false, inputParts);
            if (inputParts.empty())
                return false;

            // get current path
            InplaceArray<StringView<char>, 20> referenceParts;
            assetImportPath.slice("\\/", false, referenceParts);
            if (referenceParts.empty())
                return false;

            // remove the file name of the reference path
            referenceParts.popBack();

            // outer search (on the reference path)
            for (uint32_t i = 0; i < maxScanDepth; ++i)
            {
                // try all allowed combinations of reference path as well
                auto innerSearchDepth = std::min<uint32_t>(maxScanDepth, inputParts.size());
                for (uint32_t j = 0; j < innerSearchDepth; ++j)
                {
                    StringBuilder pathBuilder;

                    for (auto& str : referenceParts)
                    {
                        if (!pathBuilder.empty()) pathBuilder << "/";
                        pathBuilder << str;
                    }

                    auto firstInputPart = inputParts.size() - j - 1;
                    for (uint32_t k = firstInputPart; k < inputParts.size(); ++k)
                    {
                        if (!pathBuilder.empty()) pathBuilder << "/";
                        pathBuilder << inputParts[k];
                    }

                    // does the file exist ?
                    auto fileSystemPath = pathBuilder.toString();
                    if (m_assetRepository->fileExists(fileSystemPath))
                    {
                        outImportPath = fileSystemPath;
                        return true;
                    }
                }

                // ok, we didn't found anything, retry with less base directories
                if (referenceParts.empty())
                    break;
                referenceParts.popBack();
            }

            // no matching file found
            return false;
        }

        void LocalImporterInterface::followupImport(StringView<char> assetImportPath, StringView<char> depotPath, const ResourceConfiguration* config)
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

        MetadataPtr LocalImporterInterface::buildMetadata() const
        {
            auto ret = CreateSharedPtr<Metadata>();

            ret->importDependencies.reserve(m_importDependencies.size());
            for (const auto& dep : m_importDependencies)
            {
                auto& entry = ret->importDependencies.emplaceBack();
                entry.importPath = dep.assetPath;
                entry.crc = dep.fingerprint.rawValue();
            }

            ret->importFollowups.reserve(m_followupImports.size());
            for (const auto& info : m_followupImports)
            {
                auto& entry = ret->importFollowups.emplaceBack();
                entry.sourceImportPath = info.assetPath;
                entry.depotPath = info.depotPath;
                entry.configuration = info.config;
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

        void LocalImporterInterface::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text)
        {
            if (m_externalProgressTracker)
                m_externalProgressTracker->reportProgress(currentCount, totalCount, text);
        }

        //--

    } // res
} // base
