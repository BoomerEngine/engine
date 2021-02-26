/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "core/system/include/semaphoreCounter.h"
#include "core/containers/include/queue.h"
#include "core/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

//--

/// importer output
class CORE_RESOURCE_COMPILER_API IImportOutput : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_IMPORT)

public:
    virtual ~IImportOutput();

    /// schedule new content for saving
    virtual bool scheduleSave(const ResourcePtr& data, const StringBuf& depotPath) = 0;
};

//--

/// importer saving thread, saves back to depot
class CORE_RESOURCE_COMPILER_API ImportSaverThread : public IImportOutput
{
public:
    ImportSaverThread();
    ~ImportSaverThread(); // note: will kill all jobs

    /// wait for jobs to finish
    void waitUntilDone();

    /// do we have anything to save ?
    bool hasUnsavedContent() const;

    /// schedule new content for saving
    virtual bool scheduleSave(const ResourcePtr& data, const StringBuf& depotPath) override final;

private:
    struct SaveJob : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_IMPORT)

    public:
        res::ResourcePtr unsavedResource;
        StringBuf depotPath;
    };

    Queue<SaveJob*> m_saveJobQueue;
    res::ResourcePtr m_saveCurrentResource;
    SpinLock m_saveQueueLock;

    Semaphore m_saveThreadSemaphore;
    Thread m_saveThread;

    std::atomic<uint32_t> m_saveThreadRequestExit;

    ///--

    void processSavingThread();
    bool saveSingleFile(const res::ResourcePtr& data, const StringBuf& path);
};

//--

END_BOOMER_NAMESPACE_EX(res)
