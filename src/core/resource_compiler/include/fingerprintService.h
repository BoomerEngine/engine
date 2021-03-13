/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "fingerprint.h"

#include "core/io/include/timestamp.h"
#include "core/containers/include/hashMap.h"
#include "core/fibers/include/fiberSystem.h"

BEGIN_BOOMER_NAMESPACE()

//--

class ImportFingerprintCache;

//--

// a simple helper service that can compute fingerprints of files on disk
class CORE_RESOURCE_COMPILER_API SourceAssetFingerprintService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(SourceAssetFingerprintService, IService);

public:
    SourceAssetFingerprintService();
    virtual ~SourceAssetFingerprintService();

    //--

    /// compute file fingerprint of given file
    /// NOTE: this fill yield current fiber until results are available
    CAN_YIELD FingerpintCalculationStatus calculateFingerprint(StringView absolutePath, bool background, IProgressTracker* progress, SourceAssetFingerprint& outFingerprint);

    //---

private:
    static const uint32_t HEADER_MAGIC = 0x43524343;//'CRCC';

    //--

    Mutex m_cacheLock;
    RefPtr<ImportFingerprintCache> m_cache;

    StringBuf m_cacheFilePath;
    NativeTimePoint m_nextCacheWriteCheck;
    std::atomic<uint32_t> m_hasNewCacheEntries = 0;

    //--

    // a job, either loading or baking
    struct CacheJob : public IReferencable
    {
        StringBuf path;
        TimeStamp timestamp;
        SourceAssetFingerprint fingerprint;
        FingerpintCalculationStatus status = FingerpintCalculationStatus::OK;
        FiberSemaphore signal;
    };

    // synchronization for blobs being loaded/saved
    SpinLock m_activeJobMapLock;
    HashMap<StringBuf, RefWeakPtr<CacheJob>> m_activeJobMap;

    //--

    virtual bool onInitializeService(const CommandLine& cmdLine) override;
    virtual void onShutdownService() override;
    virtual void onSyncUpdate() override;

    void saveCacheIfDirty();

    //--
};

END_BOOMER_NAMESPACE()
