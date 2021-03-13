/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "fingerprint.h"

#include "sourceFileSystem.h"
#include "sourceAssetService.h"
#include "sourceFileSystemNative.h"

#include "core/resource/include/metadata.h"
#include "core/io/include/io.h"
#include "sourceAsset.h"

BEGIN_BOOMER_NAMESPACE()

//--

ConfigProperty<bool> cvAllowLocalPCImports("SourceAssets", "AllowLocalPCImports", true);
ConfigProperty<double> cvSystemTickTime("SourceAssets", "FileSystemTickInterval", 1.0);

//--

struct SourceAssetCache : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_IMPORT)

public:
    struct CacheEntry
    {
        RTTI_DECLARE_POOL(POOL_IMPORT)

    public:
        StringBuf assetImportPath;
        SourceAssetPtr asset;
        TimeStamp timestamp;
        SourceAssetFingerprint fingerprint;
        uint32_t lruTick = 0;
        uint64_t memorySize = 0;
    };

    //--

    uint64_t memoryBudgetSize = 0;
    std::atomic<uint32_t> lruTick = 0;

    uint64_t maxMemorySize = 0;
    uint64_t totalMemorySize = 0;

    Array<CacheEntry*> cacheEntries;
    HashMap<StringBuf, CacheEntry*> cacheEntriesMap;

    void ensureMemoryForAsset(uint64_t neededMemorySize);
};

//--

void SourceAssetCache::ensureMemoryForAsset(uint64_t neededMemorySize)
{
    // still in limit
    if (totalMemorySize + neededMemorySize <= memoryBudgetSize)
        return;

    // put oldest at the back
    std::sort(cacheEntries.begin(), cacheEntries.end(), [](const CacheEntry* a, const CacheEntry* b)
        {
            return a->lruTick > b->lruTick;
        });

    // if we are purging, purge a little bit more
    neededMemorySize = std::max<uint64_t>(neededMemorySize, (memoryBudgetSize * 2) / 5); // purge at least 40%

    // recalculate actual size
    totalMemorySize = 0;
    for (const auto* entry : cacheEntries)
        totalMemorySize += entry->memorySize;

    // start removing entries
    uint32_t totalEntriesRemoved = 0;
    uint64_t totalMemoryRemoved = 0;
    while (totalMemorySize + neededMemorySize > memoryBudgetSize && !cacheEntries.empty())
    {
        auto* lastEntry = cacheEntries.back();
        TRACE_INFO("Purging '{}' ({}) from source asset cache", lastEntry->assetImportPath, MemSize(lastEntry->memorySize));

        totalEntriesRemoved += 1;
        totalMemoryRemoved += lastEntry->memorySize;

        cacheEntriesMap.remove(lastEntry->assetImportPath);
        cacheEntries.popBack();
        delete lastEntry;
    }

    // print stats
    TRACE_INFO("Purged {} entrie(s) ({}) from meory cache", totalEntriesRemoved, totalMemoryRemoved);
}

//--

RTTI_BEGIN_TYPE_CLASS(SourceAssetService);
RTTI_END_TYPE();

//--

SourceAssetService::SourceAssetService()
{}

SourceAssetService::~SourceAssetService()
{}

bool SourceAssetService::fileExists(StringView assetImportPath) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->fileExists(fileSystemPath);

    return false;
}

bool SourceAssetService::translateAbsolutePath(StringView absolutePath, StringBuf& outFileSystemPath) const
{
    StringBuf bestShortPath;
    StringBuf bestPrefix;

    for (const auto& fs : m_fileSystems)
    {
        StringBuf fileSytemPath;
        if (fs.fileSystem->translateAbsolutePath(absolutePath, fileSytemPath))
        {
            if (bestShortPath.empty() || fileSytemPath.length() < bestShortPath.length())
            {
                bestShortPath = fileSytemPath;
                bestPrefix = fs.prefix;
            }
        }
    }

    if (bestShortPath.empty())
        return false;

    outFileSystemPath = TempString("{}{}", bestPrefix, bestShortPath);
    return true;
}

bool SourceAssetService::resolveContextPath(StringView assetImportPath, StringBuf& outContextPath) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->resolveContextPath(fileSystemPath, outContextPath);

    return Buffer();
}

bool SourceAssetService::enumDirectoriesAtPath(StringView assetImportPath, const std::function<bool(StringView)>& enumFunc) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->enumDirectoriesAtPath(fileSystemPath, enumFunc);

    return false;
}

bool SourceAssetService::enumFilesAtPath(StringView assetImportPath, const std::function<bool(StringView)>& enumFunc) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->enumFilesAtPath(fileSystemPath, enumFunc);

    return false;
}

bool SourceAssetService::enumRoots(const std::function<bool(StringView)>& enumFunc) const
{
    for (const auto& fs : m_fileSystems)
        if (enumFunc(fs.prefix))
            return true;
    return false;
}

//--

SourceAssetStatus SourceAssetService::checkFileStatus(StringView assetImportPath, const TimeStamp& lastKnownTimestamp, const SourceAssetFingerprint& lastKnownCRC, IProgressTracker* progress)
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->checkFileStatus(fileSystemPath, lastKnownTimestamp, lastKnownCRC, progress);

    return SourceAssetStatus::Missing;
}

//--

void SourceAssetService::collectImportConfiguration(ResourceConfiguration* config, StringView sourceAssetPath)
{
    // TODO
}

//--

Buffer SourceAssetService::loadRawContent(StringView assetImportPath, TimeStamp& outTimestamp, SourceAssetFingerprint& outFingerprint) const
{
    StringView fileSystemPath;
    if (const auto* fs = resolveFileSystem(assetImportPath, fileSystemPath))
        return fs->loadFileContent(fileSystemPath, outTimestamp, outFingerprint);

    return Buffer();
}

SourceAssetPtr SourceAssetService::loadAssetUncached(StringView assetImportPath, TimeStamp& outTimestamp, SourceAssetFingerprint& outFingerprint)
{
    // load content to buffer
    auto contentData = loadRawContent(assetImportPath, outTimestamp, outFingerprint);
    if (contentData)
    {
        /// get physical path on disk
        StringBuf contextPath(assetImportPath);
        resolveContextPath(assetImportPath, contextPath);

        // load the content
        return ISourceAssetLoader::LoadFromMemory(assetImportPath, contextPath, contentData);
    }

    // failed to load
    return nullptr;
}

SourceAssetPtr SourceAssetService::loadAssetCached(StringView assetImportPath, TimeStamp& outTimestamp, SourceAssetFingerprint& outFingerprint)
{
   const auto lruTick = m_cache->lruTick++; // capture the time

    auto lock = CreateLock(m_cacheLock);

    auto keyPath = StringBuf(assetImportPath).toLower();

    // find in cache
    {
        SourceAssetCache::CacheEntry* entry = nullptr;
        m_cache->cacheEntriesMap.find(keyPath, entry);
        if (entry && entry->asset)
        {
            // recheck file status to detect changes in cached data
            const auto ret = checkFileStatus(assetImportPath, entry->timestamp, entry->fingerprint);
            if (ret == SourceAssetStatus::UpToDate)
            {
                // use asset as is
                outTimestamp = entry->timestamp;
                outFingerprint = entry->fingerprint;
                return entry->asset;
            }
        }
    }

    // load content to buffer
    TimeStamp assetContentTimestamp;
    SourceAssetFingerprint assetContentFingerprint;
    auto contentData = loadRawContent(assetImportPath, assetContentTimestamp, assetContentFingerprint);
    if (!contentData)
    {
        TRACE_ERROR("Failed to load content for asset '{}', no source asset will be loaded", assetImportPath);
        return nullptr;
    }

    /// get physical path on disk
    StringBuf contextPath(assetImportPath);
    resolveContextPath(assetImportPath, contextPath);

    // load the content
    auto assetPtr = ISourceAssetLoader::LoadFromMemory(assetImportPath, contextPath, contentData);

    // add to cache
    if (assetPtr->shouldCacheInMemory())
    {
        // check if we didn't get a newer entry in the mean time
        auto* currentEntry = m_cache->cacheEntriesMap[keyPath];
        if (!currentEntry || currentEntry->lruTick < lruTick)
        {
            // delete old entry to free memory in case of double load
            delete currentEntry;

            // calculate the memory used by the loaded assets
            const auto memorySize = assetPtr->calcMemoryUsage();
            m_cache->ensureMemoryForAsset(memorySize);

            // add entry to cache
            auto* entry = new SourceAssetCache::CacheEntry;
            entry->assetImportPath = keyPath;
            entry->asset = assetPtr;
            entry->timestamp = assetContentTimestamp;
            entry->fingerprint = assetContentFingerprint;
            entry->lruTick = lruTick;
            entry->memorySize = memorySize;
            m_cache->cacheEntries.pushBack(entry);
            m_cache->cacheEntriesMap[keyPath] = entry;
        }
    }

    // return loaded asset
    outFingerprint = assetContentFingerprint;
    outTimestamp = assetContentTimestamp;
    return assetPtr;
}

//--

bool SourceAssetService::onInitializeService(const CommandLine& cmdLine)
{
    createFileSystems();

    m_cache = new SourceAssetCache();

    m_nextSystemUpdate = NativeTimePoint::Now() + cvSystemTickTime.get();

    return true;
}

void SourceAssetService::onShutdownService()
{
    destroyFileSystems();

    delete m_cache;
    m_cache = nullptr;
}

void SourceAssetService::onSyncUpdate()
{
    // TODO: we can do background CRC calculation here, background checks for changed assets etc

    // update the file systems from time to time
    if (m_nextSystemUpdate.reached())
    {
        m_nextSystemUpdate = NativeTimePoint::Now() + cvSystemTickTime.get();

        for (const auto& fs : m_fileSystems)
            fs.fileSystem->update();
    }
}

//--

void SourceAssetService::createFileSystems()
{
    // create the "LOCAL" source
    if (cvAllowLocalPCImports.get())
    {
        auto& entry = m_fileSystems.emplaceBack();
        entry.fileSystem = RefNew<SourceAssetFileSystem_LocalComputer>();
        entry.prefix = StringBuf("LOCAL:");
    }
}

void SourceAssetService::destroyFileSystems()
{
    m_fileSystems.clear();
}

//--

const ISourceAssetFileSystem* SourceAssetService::resolveFileSystem(StringView assetImportPath, StringView& outFileSystemPath) const
{
    for (const auto& fs : m_fileSystems)
    {
        if (assetImportPath.beginsWith(fs.prefix))
        {
            outFileSystemPath = assetImportPath.subString(fs.prefix.length());
            return fs.fileSystem;
        }
    }
            
    return nullptr;
}

//--

END_BOOMER_NAMESPACE()
