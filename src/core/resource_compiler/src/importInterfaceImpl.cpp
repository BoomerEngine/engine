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

#include "sourceAssetService.h"

#include "core/resource/include/metadata.h"
#include "core/containers/include/path.h"

BEGIN_BOOMER_NAMESPACE()

//--

LocalImporterInterface::LocalImporterInterface(const IImportDepotLoader* depot, const StringBuf& importPath, const StringBuf& depotPath, IProgressTracker* externalProgressTracker, const ResourceConfiguration* config)
    : m_importPath(importPath)
    , m_depotPath(depotPath)
    , m_externalProgressTracker(externalProgressTracker)
    , m_depot(depot)
    , m_configuration(AddRef(config))
{
    ASSERT(config != nullptr)
}

LocalImporterInterface::~LocalImporterInterface()
{}

const IResource* LocalImporterInterface::existingData() const
{
    auto lock = CreateLock(m_loadedOriginalDataLock);

    if (!m_loadedOriginalData)
        m_loadedOriginalData = m_depot->loadExistingResource(m_depotPath);

    return m_loadedOriginalData;
}

const StringBuf& LocalImporterInterface::queryResourcePath() const
{
    return m_depotPath;
}

const StringBuf& LocalImporterInterface::queryImportPath() const
{
    return m_importPath;
}

const ResourceConfiguration* LocalImporterInterface::queryConfigrationTypeless() const
{
    return m_configuration;
}

ResourceID LocalImporterInterface::queryResourceID(StringView depotPath) const
{
    return m_depot->queryResourceID(depotPath);
}

Buffer LocalImporterInterface::loadSourceFileContent(StringView assetImportPath, bool reportAsDependency)
{
    TimeStamp timestamp;
    SourceAssetFingerprint fingerprint;
    if (auto ret = GetService<SourceAssetService>()->loadRawContent(assetImportPath, timestamp, fingerprint))
    {
        if (reportAsDependency)
            reportImportDependency(assetImportPath, timestamp, fingerprint);
        return ret;
    }

    return Buffer();
}

SourceAssetPtr LocalImporterInterface::loadSourceAsset(StringView assetImportPath, bool reportAsDependency, bool allowCaching)
{
    TimeStamp timestamp;
    SourceAssetFingerprint fingerprint;

    if (allowCaching)
    {
        if (auto ret = GetService<SourceAssetService>()->loadAssetCached(assetImportPath, timestamp, fingerprint))
        {
            if (reportAsDependency)
                reportImportDependency(assetImportPath, timestamp, fingerprint);
            return ret;
        }
    }
    else
    {
        if (auto ret = GetService<SourceAssetService>()->loadAssetUncached(assetImportPath, timestamp, fingerprint))
        {
            if (reportAsDependency)
                reportImportDependency(assetImportPath, timestamp, fingerprint);
            return ret;
        }
    }

    return nullptr;
}

void LocalImporterInterface::reportImportDependency(StringView assetImportPath, const TimeStamp& timestamp, const SourceAssetFingerprint& fingerprint)
{
    auto lock = CreateLock(m_dependenciesLock);

    const auto assetKey = StringBuf(assetImportPath).toLower();

    if (m_dependenciesSet.insert(assetKey))
    {
        auto& entry = m_dependencies.emplaceBack();
        entry.assetPath = StringBuf(assetImportPath);
        entry.fingerprint = fingerprint;

        TRACE_SPAM("Reported '{}' as import dependency, last modified at {}, fingerprint: {}", assetImportPath, timestamp, fingerprint);
    }
}

//--

bool LocalImporterInterface::findSourceFile(StringView assetImportPath, StringView inputPath, StringBuf& outImportPath, uint32_t maxScanDepth /*= 2*/) const
{
    return ScanRelativePaths(assetImportPath, inputPath, maxScanDepth, outImportPath, [this](StringView testPath)
        {
            return GetService<SourceAssetService>()->fileExists(testPath);
        });
}

ResourceID LocalImporterInterface::followupImport(StringView assetImportPath, StringView depotPath, ResourceClass cls, const ResourceConfiguration* config)
{
    DEBUG_CHECK_RETURN_EX_V(assetImportPath, "Invalid asset path", ResourceID());
    DEBUG_CHECK_RETURN_EX_V(depotPath, "Invalid depot path", ResourceID());

    const auto depotKey = StringBuf(depotPath).toLower();

    // reuse existing reference
    ResourceID id;
    if (m_followupImportsSet.find(depotKey, id))
        return id;

    // find the actual importable class
    {
        InplaceArray<ResourceClass, 10> importableClasses;
        IResourceImporter::ListImportableResourceClassesForExtension(assetImportPath.extensions(), importableClasses);
        if (importableClasses.empty())
        {
            TRACE_INFO("Followup import '{}' cannot be created because there are no importers for '{}'", depotPath, assetImportPath);
            return ResourceID();
        }

        // if we don't have our class specified directly find class that actually matches us best
        // NOTE: may be problematic
        if (!importableClasses.contains(cls))
        {
            TRACE_INFO("Followup import '{}' cannot be created because there are no importers for '{}' that can import into '{}' directly", depotPath, assetImportPath, cls);
            return ResourceID();
        }
    }

    // try to use existing resource key
    const auto metadataPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);
    if (const auto metadata = m_depot->loadExistingMetadata(metadataPath))
    {
        if (!metadata->ids.empty())
        {
            id = metadata->ids.front();
            TRACE_INFO("Followup import '{}' already exists and has ID '{}'", depotPath, id);
        }
        else
        {
            id = ResourceID::Create();
            TRACE_INFO("Followup import '{}' exists but has no ID assigned, assigned new ID {}", depotPath, id);
        }
    }
    else
    {
        id = ResourceID::Create();
        TRACE_INFO("Followup import '{}' does not exist yet, assigned new ID {}", depotPath, id);
    }

    // store
    auto& entry = m_followupImports.emplaceBack();
    entry.assetPath = StringBuf(assetImportPath);
    entry.depotPath = StringBuf(depotPath);
    entry.config = AddRef(config);
    entry.resourceClass = cls;
    entry.id = id;

    // store in map
    m_followupImportsSet[depotKey] = id;
    return id;
}

//--

bool LocalImporterInterface::findDepotFile(StringView depotReferencePath, StringView depotSearchPath, StringView searchFileName, StringBuf& outDepotPath, ResourceID* outID, uint32_t maxScanDepth) const
{
    StringBuf depotFinalSearchPath;
    if (!ApplyRelativePath(depotReferencePath, depotSearchPath, depotFinalSearchPath))
        return false;

    return m_depot->findFile(depotFinalSearchPath, searchFileName, maxScanDepth, outDepotPath, outID);
}

bool LocalImporterInterface::checkDepotFile(StringView depotPath) const
{
    return m_depot->fileExists(depotPath);
}

bool LocalImporterInterface::checkSourceFile(StringView assetImportPath) const
{
    return GetService<SourceAssetService>()->fileExists(assetImportPath);
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

END_BOOMER_NAMESPACE()
