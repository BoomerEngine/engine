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

        LocalImporterInterface::LocalImporterInterface(SourceAssetRepository* assetRepository, const IResource* originalData, const StringBuf& importPath, const ResourcePath& depotPath, const ResourceMountPoint& depotMountPoint, IProgressTracker* externalProgressTracker, const Array<ResourceConfigurationPtr>& configurations)
            : m_originalData(originalData)
            , m_importPath(importPath)
            , m_depotPath(depotPath)
            , m_depotMountPoint(depotMountPoint)
            , m_externalProgressTracker(externalProgressTracker)
            , m_assetRepository(assetRepository)
        {
            // use given configurations
            m_configurations = configurations;

            // if we have existing resource than use all configurations from it, regardless of our stuff
            // TODO: merge ? 
            if (originalData)
            {
                if (const auto metadata = originalData->metadata())
                {
                    for (const auto& config : metadata->importConfigurations)
                        insertConfiguration(config); // NOTE: this will replace any configuration we already have
                }
            }
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

        const IResourceConfiguration* LocalImporterInterface::queryConfigration(SpecificClassType<IResourceConfiguration> configClass) const
        {
            DEBUG_CHECK_EX(configClass && !configClass->isAbstract(), "Invalid resource configuration class");
            if (!configClass || configClass->isAbstract())
                return nullptr;

            // check given or previous configurations
            for (const auto& config : m_configurations)
                if (config->is(configClass))
                    return config;

            // temp list 
            {
                auto lock = CreateLock(m_tempConfigurationsLock);

                for (const auto& config : m_tempConfigurations)
                    if (config->is(configClass))
                        return config;

                auto config = configClass.create();
                m_tempConfigurations.pushBack(config);
                return config;
            }
        }

        Buffer LocalImporterInterface::loadSourceFileContent(StringView<char> assetImportPath) const
        {
            uint64_t crc = 0;
            if (auto ret = m_assetRepository->loadSourceFileContent(assetImportPath, crc))
            {
                const_cast<LocalImporterInterface*>(this)->reportImportDependency(assetImportPath, crc);
                return ret;
            }

            return Buffer();
        }

        SourceAssetPtr LocalImporterInterface::loadSourceAsset(StringView<char> assetImportPath, SpecificClassType<ISourceAsset> sourceAssetClass) const
        {
            uint64_t crc = 0;
            if (auto ret = m_assetRepository->loadSourceAsset(assetImportPath, sourceAssetClass, crc))
            {
                const_cast<LocalImporterInterface*>(this)->reportImportDependency(assetImportPath, crc);
                return ret;
            }

            return nullptr;
        }

        void LocalImporterInterface::reportImportDependency(StringView<char> assetImportPath, uint64_t crc)
        {
            auto lock = CreateLock(m_importDependenciesLock);

            const auto assetKey = StringBuf(assetImportPath).toLower();

            if (m_importDependenciesSet.insert(assetKey))
            {
                auto& entry = m_importDependencies.emplaceBack();
                entry.assetPath = StringBuf(assetImportPath);
                entry.crc = crc;
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

        void LocalImporterInterface::followupImport(StringView<char> assetImportPath, StringView<char> depotPath, const Array<ResourceConfigurationPtr>& config /*= Array<ResourceConfigurationPtr>()*/)
        {
            if (assetImportPath && depotPath)
            {
                const auto depotKey = StringBuf(depotPath).toLower();

                if (m_followupImportsSet.insert(depotKey))
                {
                    auto& entry = m_followupImports.emplaceBack();
                    entry.assetPath = StringBuf(assetImportPath);
                    entry.depotPath = StringBuf(depotPath);
                    entry.config = config;
                }
                else
                {
                    TRACE_WARNING("Followup import '{}' already specified", depotPath);
                }
            }
        }

        //--

        void LocalImporterInterface::insertConfiguration(IResourceConfiguration* ptr)
        {
            for (uint32_t i=0; i<m_configurations.size(); ++i)
            {
                if (m_configurations[i]->cls() == ptr->cls())
                {
                    m_configurations[i] = AddRef(ptr);
                    return;
                }
            }

            m_configurations.emplaceBack(AddRef(ptr));
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
                entry.crc = dep.crc;
            }

            ret->importFollowups.reserve(m_followupImports.size());
            for (const auto& info : m_followupImports)
            {
                auto& entry = ret->importFollowups.emplaceBack();
                entry.sourceImportPath = info.assetPath;
                entry.depotPath = info.depotPath;

                for (const auto& cfg : info.config)
                {
                    entry.configuration.pushBack(cfg);
                    cfg->parent(ret);
                }
            }

            for (const auto& cfg : m_configurations)
            {
                if (auto configCopy = rtti_cast<IResourceConfiguration>(cfg->clone(ret)))
                    ret->importConfigurations.pushBack(configCopy);
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
