/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "fingerprint.h"
#include "fingerprintService.h"
#include "fingerprintCache.h"

#include "core/io/include/asyncFileHandle.h"
#include "core/io/include/io.h"
#include "core/resource/include/fileLoader.h"
#include "core/resource/include/fileSaver.h"

BEGIN_BOOMER_NAMESPACE()

//--

static RefPtr<ImportFingerprintCache> LoadCache(StringView path)
{
    if (auto file = OpenForAsyncReading(path))
    {
        FileLoadingContext context;
           
        FileLoadingResult result;
        if (LoadFile(file, context, result))
        {
            if (auto cache = result.root<ImportFingerprintCache>())
            {
                TRACE_INFO("Fingerprint: Loaded cache with {} entrie(s)", cache->size());
                return cache;
            }
        }
    }

    TRACE_INFO("Fingerprint: Created new cache");
    return RefNew<ImportFingerprintCache>();
}

static bool SaveCache(StringView path, const RefPtr<ImportFingerprintCache>& cache)
{
    if (auto file = OpenForWriting(path))
    {
        FileSavingContext context;
        context.rootObjects.pushBack(cache);

        FileSavingResult result;
        return SaveFile(file, context, result);
    }

    return false;
}

//--

static const ConfigProperty<double> cvFingerprintCacheSaveInterval("Assets.Fingerprint", "CacheSaveInterval", 10);

//--

RTTI_BEGIN_TYPE_CLASS(SourceAssetFingerprintService);
RTTI_END_TYPE();

SourceAssetFingerprintService::SourceAssetFingerprintService()
{}

SourceAssetFingerprintService::~SourceAssetFingerprintService()
{}

bool SourceAssetFingerprintService::onInitializeService(const CommandLine& cmdLine)
{
    // determine the fingerprint cache file
    m_cacheFilePath = TempString("{}fingerprint.cache", SystemPath(PathCategory::UserConfigDir));
    TRACE_INFO("Fingerprint: Cache located at '{}'", m_cacheFilePath);

    // load/cache the cache
    m_cache = LoadCache(m_cacheFilePath);
    m_nextCacheWriteCheck = NativeTimePoint::Now() + cvFingerprintCacheSaveInterval.get();
    return true;
}

void SourceAssetFingerprintService::onShutdownService()
{
    saveCacheIfDirty();
}

void SourceAssetFingerprintService::onSyncUpdate()
{
    if (m_nextCacheWriteCheck.reached())
    {
        saveCacheIfDirty();
        m_nextCacheWriteCheck = NativeTimePoint::Now() + cvFingerprintCacheSaveInterval.get();
    }
}

void SourceAssetFingerprintService::saveCacheIfDirty()
{
    if (1 == m_hasNewCacheEntries.exchange(0))
    {
        auto lock = CreateLock(m_cacheLock);

        // save cache to file
        if (!SaveCache(m_cacheFilePath, m_cache))
        {
            TRACE_WARNING("Fingerprint: Failed to save fingerprint cache");
        }
    }
}

//--

CAN_YIELD FingerpintCalculationStatus SourceAssetFingerprintService::calculateFingerprint(StringView absolutePath, bool background, IProgressTracker* progress, SourceAssetFingerprint& outFingerprint)
{
    // first, check the file timestamp, maybe we have the data in cache
    TimeStamp timestamp;
    if (!FileTimeStamp(absolutePath, timestamp))
        return FingerpintCalculationStatus::ErrorNoFile;

    // locate in cache
    {
        auto lock = CreateLock(m_cacheLock);
        if (m_cache->findEntry(absolutePath, timestamp, outFingerprint))
            return FingerpintCalculationStatus::OK;
    }

    // enter the locked region of the operations
    auto lock = CreateLock(m_activeJobMapLock);

    // do we have a job entry already ?
    RefWeakPtr<CacheJob> existingCacheJob;
    if (m_activeJobMap.find(absolutePath, existingCacheJob))
    {
        // get the valid entry (note: we may have been released)
        if (auto validCacheJob = existingCacheJob.lock())
        {
            // release the lock to allow other threads to join waiting for this resource
            lock.release();

            // wait for the signal from the thread that created the job
            TRACE_INFO("CRC cache contention on '{}', waiting for previous job '{}'", absolutePath, validCacheJob->path);
            WaitForFence(validCacheJob->signal);

            // we have failed
            if (validCacheJob->status == FingerpintCalculationStatus::OK)
                outFingerprint = validCacheJob->fingerprint;
            return validCacheJob->status;
        }
    }

    // create a cache job entry
    auto validCacheJob = RefNew<CacheJob>();
    validCacheJob->path = StringBuf(absolutePath);
    validCacheJob->timestamp = timestamp;
    validCacheJob->signal = CreateFence("SourceAssetFingerprintServiceCalcJob");
    m_activeJobMap[validCacheJob->path] = validCacheJob;

    // release lock so other threads may join waiting
    lock.release();

    // open the file and calculate the fingerprint
    if (auto file = OpenForAsyncReading(absolutePath))
    {
        validCacheJob->status = CalculateFileFingerprint(file, !background, progress, validCacheJob->fingerprint);
    }
    else
    {
        validCacheJob->status = FingerpintCalculationStatus::ErrorNoFile;
    }

    // done, put results in in cache
    if (validCacheJob->status == FingerpintCalculationStatus::OK)
    {
        auto lock = CreateLock(m_cacheLock);
        m_cache->storeEntry(absolutePath, validCacheJob->timestamp, validCacheJob->fingerprint);
        m_hasNewCacheEntries.exchange(1);
    }

    // signal dependencies
    SignalFence(validCacheJob->signal);

    // copy result
    if (validCacheJob->status == FingerpintCalculationStatus::OK)
        outFingerprint = validCacheJob->fingerprint;
    return validCacheJob->status;
}

//--

END_BOOMER_NAMESPACE()
