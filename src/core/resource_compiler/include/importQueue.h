/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "importer.h"

#include "core/app/include/localService.h"
#include "core/socket/include/tcpServer.h"
#include "core/resource/include/loader.h"
#include "core/system/include/semaphoreCounter.h"

BEGIN_BOOMER_NAMESPACE()

//--

class Importer;
class IImportOutput;
class ImportQueueDepotChecker;
struct ImportJobInfo;

//--

/// notification interface for the queue
class CORE_RESOURCE_COMPILER_API IImportQueueCallbacks : public NoCopy
{
public:
    virtual ~IImportQueueCallbacks();

    virtual void queueJobAdded(const ImportJobInfo& info) {};
    virtual void queueJobStarted(StringView depotPath) {};
    virtual void queueJobFinished(StringView depotPath, ImportStatus status, double timeTaken) {};
    virtual void queueJobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) {};
};

//--

/// helper class to retrieve existing content of a resource
class CORE_RESOURCE_COMPILER_API IImportDepotLoader : public NoCopy
{
public:
    virtual ~IImportDepotLoader();

    // load metadata of existing resource, should not be cached or reused
    virtual ResourceMetadataPtr loadExistingMetadata(StringView depotPath) const = 0;

    // load existing content of a resource, should not be cached or reused
    virtual ResourcePtr loadExistingResource(StringView depotPath) const = 0;

    // check if file exists
    virtual bool depotFileExists(StringView depotPath) const = 0;

    // find depot file
    virtual bool depotFindFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath, ResourceID& outFoundResourceID) const = 0;
};

//--

/// a helper class that processes the list of import jobs
class CORE_RESOURCE_COMPILER_API ImportQueue : public NoCopy
{
public:
    ImportQueue(SourceAssetRepository* assets, IImportDepotLoader* loader, IImportOutput* saver, IImportQueueCallbacks* callbacks);
    ~ImportQueue();

    //--

    /// add a import job to the queue
    /// NOTE: already added jobs are ignored
    void scheduleJob(const ImportJobInfo& job);

    /// process next job from the list, returns false if there are no more jobs
    /// NOTE: thread safe
    bool processNextJob(IProgressTracker* progressTracker);

    //--

private:
    IImportDepotLoader* m_loader = nullptr;
    IImportOutput* m_saver = nullptr;
    SourceAssetRepository* m_assets = nullptr;

    UniquePtr<ImportQueueDepotChecker> m_importerDepotChecker;
    UniquePtr<Importer> m_importer;

    //--

    struct LocalJobInfo : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_IMPORT)

    public:
        ImportJobInfo info;
    };

    SpinLock m_jobLock;
    Array<LocalJobInfo*> m_jobsList;
    HashMap<StringBuf, LocalJobInfo*> m_jobsMap;
    Queue<LocalJobInfo*> m_jobQueue;

    volatile uint32_t m_numTotalJobsDone = 0;
    volatile uint32_t m_numTotalJobsScheduled = 0;

    const LocalJobInfo* popNextJob();

    //--

    IImportQueueCallbacks* m_callbacks = nullptr;
};

//--

END_BOOMER_NAMESPACE()
