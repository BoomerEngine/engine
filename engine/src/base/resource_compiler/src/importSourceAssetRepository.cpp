/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importSourceAsset.h"
#include "importSourceAssetRepository.h"
#include "importFileService.h"
#include "importFileFingerprint.h"
#include "base/io/include/timestamp.h"

namespace base
{
    namespace res
    {
        //--

        ConfigProperty<uint32_t> cvSourceRepositoryMemoryCachesizeMB("SourceAssets", "RepositoryMemoryCachesizeMB", 16 * 1024); // 16GB 

        //--

        SourceAssetRepository::SourceAssetRepository(ImportFileService* fileService, uint64_t maxCachedSourceAssetsMemoryBudget)
            : m_fileService(fileService)
            , m_memoryBudgetSize(maxCachedSourceAssetsMemoryBudget)
        {
            if (m_memoryBudgetSize == 0)
                m_memoryBudgetSize = (uint64_t)cvSourceRepositoryMemoryCachesizeMB.get() * 1048576ULL;

            // TODO: limit to half of available free memory
        }

        SourceAssetRepository::~SourceAssetRepository()
        {
            clearCache();

            // TODO: print stats
        }

        void SourceAssetRepository::clearCache()
        {
            m_cacheEntries.clearPtr();
            m_cacheEntriesMap.clear();
            m_totalMemorySize = 0;
        }

        bool SourceAssetRepository::fileExists(StringView assetImportPath) const
        {
            return m_fileService->fileExists(assetImportPath);
        }

        Buffer SourceAssetRepository::loadSourceFileContent(StringView assetImportPath, io::TimeStamp& outTimestamp, ImportFileFingerprint& outFingerprint)
        {
            return m_fileService->loadFileContent(assetImportPath, outTimestamp, outFingerprint);
        }

        CAN_YIELD SourceAssetStatus SourceAssetRepository::checkFileStatus(StringView assetImportPath, const io::TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownFingerprint, IProgressTracker* progress)
        {
            return m_fileService->checkFileStatus(assetImportPath, lastKnownTimestamp, lastKnownFingerprint, progress);
        }

        ResourceConfigurationPtr SourceAssetRepository::compileBaseResourceConfiguration(StringView assetImportPath, SpecificClassType<ResourceConfiguration> configurationClass)
        {
            return m_fileService->compileBaseResourceConfiguration(assetImportPath, configurationClass);
        }

        SourceAssetPtr SourceAssetRepository::loadSourceAsset(StringView assetImportPath, io::TimeStamp& outTimestamp, ImportFileFingerprint& outFingerprint)
        {
            auto lock = CreateLock(m_lock);

            auto keyPath = StringBuf(assetImportPath).toLower();

            // find in cache
            CacheEntry* entry = nullptr;
            m_cacheEntriesMap.findSafe(keyPath, entry);
            if (entry)
            {
                if (entry->asset)
                {
                    outTimestamp = entry->timestamp;
                    outFingerprint = entry->fingerprint;
                    entry->lruTick = m_lruTick++;
                    return entry->asset;
                }
            }

            // load content to buffer
            io::TimeStamp assetContentTimestamp;
            ImportFileFingerprint assetContentFingerprint;
            auto contentData = loadSourceFileContent(assetImportPath, assetContentTimestamp, assetContentFingerprint);
            if (!contentData)
            {
                TRACE_ERROR("Failed to load content for asset '{}', no source asset will be loaded", assetImportPath);
                return nullptr;
            }

            /// get physical path on disk
            StringBuf contextPath(assetImportPath);
            m_fileService->resolveContextPath(assetImportPath, contextPath);

            // load the content
            auto assetPtr = ISourceAssetLoader::LoadFromMemory(assetImportPath, contextPath, contentData);

            // add to cache
            if (assetPtr->shouldCacheInMemory())
            {
                // calculate the memory used by the loaded assets
                const auto memorySize = assetPtr->calcMemoryUsage();
                ensureMemoryForAsset(memorySize);

                // add entry to cache
                auto* entry = new CacheEntry;
                entry->assetImportPath = keyPath;
                entry->asset = assetPtr;
                entry->timestamp = assetContentTimestamp;
                entry->fingerprint = assetContentFingerprint;
                entry->lruTick = m_lruTick++;
                entry->memorySize = memorySize;
                m_cacheEntries.pushBack(entry);
                m_cacheEntriesMap[keyPath] = entry;
            }

            // return loaded asset
            outFingerprint = assetContentFingerprint;
            outTimestamp = assetContentTimestamp;
            return assetPtr;
        }

        //--

        void SourceAssetRepository::ensureMemoryForAsset(uint64_t neededMemorySize)
        {
            // still in limit
            if (m_totalMemorySize + neededMemorySize <= m_memoryBudgetSize)
                return;

            // put oldest at the back
            std::sort(m_cacheEntries.begin(), m_cacheEntries.end(), [](const CacheEntry* a, const CacheEntry* b)
                {
                    return a->lruTick > b->lruTick;
                });

            // if we are purging, purge a little bit more
            neededMemorySize = std::max<uint64_t>(neededMemorySize, (m_memoryBudgetSize * 2) / 5); // purge at least 40%

            // recalculate actual size
            m_totalMemorySize = 0;
            for (const auto* entry : m_cacheEntries)
                m_totalMemorySize += entry->memorySize;

            // start removing entries
            uint32_t totalEntriesRemoved = 0;
            uint64_t totalMemoryRemoved = 0;
            while (m_totalMemorySize + neededMemorySize > m_memoryBudgetSize && !m_cacheEntries.empty())
            {
                auto* lastEntry = m_cacheEntries.back();
                TRACE_INFO("Purging '{}' ({}) from memory cache", lastEntry->assetImportPath, MemSize(lastEntry->memorySize));

                totalEntriesRemoved += 1;
                totalMemoryRemoved += lastEntry->memorySize;

                m_cacheEntriesMap.remove(lastEntry->assetImportPath);
                m_cacheEntries.popBack();
                delete lastEntry;
            }

            // print stats
            TRACE_INFO("Purged {} entrie(s) ({}) from meory cache", totalEntriesRemoved, totalMemoryRemoved);
        }

        //--

    } // res
} // base
