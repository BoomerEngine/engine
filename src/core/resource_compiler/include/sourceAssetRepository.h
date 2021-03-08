/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "core/containers/include/hashMap.h"
#include "core/io/include/timestamp.h"

#include "fingerprint.h"

BEGIN_BOOMER_NAMESPACE()

//--
        
/// repository of source assets (basic loader + cache)
class CORE_RESOURCE_COMPILER_API SourceAssetRepository : public NoCopy
{
public:
    SourceAssetRepository(SourceAssetFingerprint* fileService, uint64_t maxCachedSourceAssetsMemoryBudget = 0);
    virtual ~SourceAssetRepository();

    ///---

    /// clear the internal cache
    void clearCache();

    /// check if file exists
    bool fileExists(StringView assetImportPath) const;

    /// load raw data for an asset, usually not cached
    Buffer loadSourceFileContent(StringView assetImportPath, TimeStamp& outTimestamp, SourceAssetFingerprint& outFingerprint);

    /// load source asset
    SourceAssetPtr loadSourceAsset(StringView assetImportPath, TimeStamp& outTimestamp, SourceAssetFingerprint& outFingerprint);

    /// load/create base resource configuration
    void collectImportConfiguration(ResourceConfiguration* config, StringView sourceAssetPath);

    // check status of a file
    CAN_YIELD SourceAssetStatus checkFileStatus(StringView assetImportPath, const TimeStamp& lastKnownTimestamp, const SourceAssetFingerprint& lastKnownFingerprint, IProgressTracker* progress);
            
    ///---

private:
    Mutex m_lock;

    SourceAssetFingerprint* m_fileService = nullptr;

    struct CacheEntry : public NoCopy
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

    uint64_t m_memoryBudgetSize = 0;
    std::atomic<uint32_t> m_lruTick = 0;

    uint64_t m_maxMemorySize = 0;
    uint64_t m_totalMemorySize = 0;
    Array<CacheEntry*> m_cacheEntries;
    HashMap<StringBuf, CacheEntry*> m_cacheEntriesMap;

    //--

    void ensureMemoryForAsset(uint64_t neededMemorySize);
};

//--

END_BOOMER_NAMESPACE()
