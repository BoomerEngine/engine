/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "core/app/include/localService.h"
#include "core/socket/include/tcpServer.h"
#include "core/resource/include/loader.h"
#include "core/system/include/semaphoreCounter.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// cooker saving thread, saves to absolute paths
class CORE_RESOURCE_COMPILER_API CookerSaveThread : public NoCopy
{
public:
    CookerSaveThread();
    ~CookerSaveThread(); // note: will kill all jobs

    /// wait for jobs to finish
    void waitUntilDone();

    /// schedule new content for saving
    bool scheduleSave(const ResourcePtr& data, StringView path);

private:
    struct SaveJob : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_IO)

    public:
        ResourcePtr unsavedResource;
        StringBuf absoultePath;
    };

    Queue<SaveJob*> m_saveJobQueue;
    ResourcePtr m_saveCurrentResource;
    SpinLock m_saveQueueLock;

    Semaphore m_saveThreadSemaphore;
    Thread m_saveThread;

    std::atomic<uint32_t> m_saveThreadRequestExit;

    ///--

    void processSavingThread();
    bool saveSingleFile(const ResourcePtr& data, StringView path);
};

//--

END_BOOMER_NAMESPACE()
