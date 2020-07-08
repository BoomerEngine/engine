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

        bool SourceAssetRepository::fileExists(StringView<char> assetImportPath, uint64_t* outCRC) const
        {
            return m_fileService->fileExists(assetImportPath, outCRC);
        }

        Buffer SourceAssetRepository::loadSourceFileContent(StringView<char> assetImportPath, uint64_t& outCRC)
        {
            return m_fileService->loadFileContent(assetImportPath, outCRC);
        }

        SourceAssetPtr SourceAssetRepository::loadSourceAsset(StringView<char> assetImportPath, SpecificClassType<ISourceAsset> contentType, uint64_t& outCRC)
        {
            DEBUG_CHECK_EX(contentType && !contentType->isAbstract(), "Invalid content type class");

            if (!contentType || contentType->isAbstract())
                return nullptr;

            auto lock = CreateLock(m_lock);

            auto keyPath = StringBuf(assetImportPath).toLower();

            // find in cache
            CacheEntry* entry = nullptr;
            m_cacheEntriesMap.findSafe(keyPath, entry);
            if (entry)
            {
                DEBUG_CHECK_EX(entry->asset && entry->asset->is(contentType), "Cached asset entry is invalid");
                if (entry->asset && entry->asset->is(contentType))
                {
                    outCRC = entry->crc;
                    entry->lruTick = m_lruTick++;
                    return entry->asset;
                }
            }

            // load content to buffer
            uint64_t assetContentCRC = 0;
            auto contentData = loadSourceFileContent(assetImportPath, assetContentCRC);
            if (!contentData)
            {
                TRACE_ERROR("Failed to load content for asset '{}', no source asset will be loaded", assetImportPath);
                return nullptr;
            }

            // create the asset
            auto assetPtr = contentType.create();
            if (!assetPtr->loadFromMemory(contentData))
            {
                TRACE_ERROR("Failed to load source asset '{}' from loaded data", assetImportPath);
                return nullptr;
            }

            // add to cache
            if (assetPtr->shouldCacheInMemory())
            {
                // calculate the memory used by the loaded assets
                const auto memorySize = assetPtr->calcMemoryUsage();
                ensureMemoryForAsset(memorySize);

                // add entry to cache
                auto* entry = MemNew(CacheEntry).ptr;
                entry->assetImportPath = keyPath;
                entry->asset = assetPtr;
                entry->crc = assetContentCRC;
                entry->lruTick = m_lruTick++;
                entry->memorySize = memorySize;
                m_cacheEntries.pushBack(entry);
                m_cacheEntriesMap[keyPath] = entry;
            }

            // return loaded asset
            outCRC = assetContentCRC;
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
                MemDelete(lastEntry);
            }

            // print stats
            TRACE_INFO("Purged {} entrie(s) ({}) from meory cache", totalEntriesRemoved, totalMemoryRemoved);
        }

        //--

    } // res
} // base
