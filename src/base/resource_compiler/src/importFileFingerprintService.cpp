/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importFileFingerprint.h"
#include "importFileFingerprintService.h"
#include "importFileFingerprintCache.h"
#include "base/io/include/ioAsyncFileHandle.h"
#include "base/io/include/ioSystem.h"
#include "base/resource/include/resourceFileLoader.h"
#include "base/resource/include/resourceFileSaver.h"

BEGIN_BOOMER_NAMESPACE(base::res)

//--

static RefPtr<ImportFingerprintCache> LoadCache(StringView path)
{
    if (auto file = base::io::OpenForAsyncReading(path))
    {
        FileLoadingContext context;
                
        if (LoadFile(file, context))
        {
            if (auto cache = context.root<ImportFingerprintCache>())
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
    if (auto file = base::io::OpenForWriting(path))
    {
        FileSavingContext context;
        context.rootObject.pushBack(cache);

        return SaveFile(file, context);
    }

    return false;
}

//--

static const ConfigProperty<double> cvFingerprintCacheSaveInterval("Assets.Fingerprint", "CacheSaveInterval", 10);

//--

RTTI_BEGIN_TYPE_CLASS(ImportFileFingerprintService);
RTTI_END_TYPE();

ImportFileFingerprintService::ImportFileFingerprintService()
{}

ImportFileFingerprintService::~ImportFileFingerprintService()
{}

app::ServiceInitializationResult ImportFileFingerprintService::onInitializeService(const app::CommandLine& cmdLine)
{
    // determine the fingerprint cache file
    m_cacheFilePath = TempString("{}fingerprint.cache", base::io::SystemPath(io::PathCategory::UserConfigDir));
    TRACE_INFO("Fingerprint: Cache located at '{}'", m_cacheFilePath);

    // load/cache the cache
    m_cache = LoadCache(m_cacheFilePath);
    m_nextCacheWriteCheck = NativeTimePoint::Now() + cvFingerprintCacheSaveInterval.get();
    return app::ServiceInitializationResult::Finished;
}

void ImportFileFingerprintService::onShutdownService()
{
    saveCacheIfDirty();
}

void ImportFileFingerprintService::onSyncUpdate()
{
    if (m_nextCacheWriteCheck.reached())
    {
        saveCacheIfDirty();
        m_nextCacheWriteCheck = NativeTimePoint::Now() + cvFingerprintCacheSaveInterval.get();
    }
}

void ImportFileFingerprintService::saveCacheIfDirty()
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

CAN_YIELD FingerpintCalculationStatus ImportFileFingerprintService::calculateFingerprint(StringView absolutePath, bool background, IProgressTracker* progress, ImportFileFingerprint& outFingerprint)
{
    // first, check the file timestamp, maybe we have the data in cache
    io::TimeStamp timestamp;
    if (!base::io::FileTimeStamp(absolutePath, timestamp))
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
            Fibers::GetInstance().waitForCounterAndRelease(validCacheJob->signal);

            // we have failed
            if (validCacheJob->status == FingerpintCalculationStatus::OK)
                outFingerprint = validCacheJob->fingerprint;
            return validCacheJob->status;
        }
    }

    // create a cache job entry
    auto validCacheJob = base::RefNew<CacheJob>();
    validCacheJob->path = StringBuf(absolutePath);
    validCacheJob->timestamp = timestamp;
    validCacheJob->signal = Fibers::GetInstance().createCounter("ImportFileFingerprintServiceCalcJob");
    m_activeJobMap[validCacheJob->path] = validCacheJob;

    // release lock so other threads may join waiting
    lock.release();

    // open the file and calculate the fingerprint
    if (auto file = base::io::OpenForAsyncReading(absolutePath))
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
    Fibers::GetInstance().signalCounter(validCacheJob->signal);

    // copy result
    if (validCacheJob->status == FingerpintCalculationStatus::OK)
        outFingerprint = validCacheJob->fingerprint;
    return validCacheJob->status;
}

//--

END_BOOMER_NAMESPACE(base::res)
