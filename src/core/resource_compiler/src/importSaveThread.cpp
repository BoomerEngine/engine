/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importSaveThread.h"
#include "core/system/include/thread.h"
#include "core/resource/include/fileSaver.h"
#include "core/io/include/fileHandle.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE()

//--

IImportOutput::~IImportOutput()
{}

//--

ImportSaverThread::ImportSaverThread()
    : m_saveThreadSemaphore(0, INT_MAX)
    , m_saveThreadRequestExit(0)
{
    // start the saving thread
    ThreadSetup setup;
    setup.m_priority = ThreadPriority::AboveNormal;
    setup.m_name = "ImporterSavingThread";
    setup.m_stackSize = 10 * 1024 * 1024; // saving requires large stack size
    setup.m_function = [this]() { processSavingThread(); };
    m_saveThread.init(setup);
}

ImportSaverThread::~ImportSaverThread()
{
    m_saveThreadRequestExit.exchange(1);

    // wait for thread to finish
    {
        ScopeTimer timer;
        m_saveThreadRequestExit.exchange(1);
        m_saveThread.close();
        TRACE_INFO("Cooker saving thread closed in '{}'", timer);
    }

    // cleanup all jobs
    {
        auto saveLock = CreateLock(m_saveQueueLock);
        while (!m_saveJobQueue.empty())
        {
            auto* job = m_saveJobQueue.top();
            m_saveJobQueue.pop();
            delete job;
        }
    }
}

bool ImportSaverThread::hasUnsavedContent() const
{
    auto saveLock = CreateLock(m_saveQueueLock);
    if (!m_saveJobQueue.empty())
        return false;
    if (m_saveCurrentResource)
        return false;
    return true;
}

void ImportSaverThread::waitUntilDone()
{
    ScopeTimer timer;

    bool printedMessage = false;
    for (;;)
    {
        uint32_t queueSize = 0;
        {
            auto saveLock = CreateLock(m_saveQueueLock);
            queueSize = m_saveJobQueue.size();
            if (m_saveJobQueue.empty() && !m_saveCurrentResource)
                break;
        }

        if (!printedMessage)
        {
            printedMessage = true;
            TRACE_INFO("Waiting for save thread to finish saving ({} in queue)...", queueSize);
        }
        else
        {
            TRACE_INFO("Still waiting ({} in queue)...", queueSize);
        }
        Sleep(500);
    }

    if (printedMessage)
        TRACE_INFO("Saving finished in extra {}", timer);
}

bool ImportSaverThread::scheduleSave(const ResourcePtr& data, const StringBuf& depotPath)
{
    DEBUG_CHECK_EX(!depotPath.empty(), "Invalid path");
    DEBUG_CHECK_EX(data, "Invalid data to save");

    // add to queue
    {
        auto saveLock = CreateLock(m_saveQueueLock);

        auto job = new SaveJob;
        job->depotPath = depotPath;
        job->unsavedResource = data;
        m_saveJobQueue.push(job);
    }

    // wakeup thread
    m_saveThreadSemaphore.release(1);
    return true;
}

bool ImportSaverThread::saveSingleFile(const ResourcePtr& data, const StringBuf& path)
{
    // create staged writer for the file
    if (auto file = GetService<DepotService>()->createFileWriter(path))
    {
        FileSavingContext context;
        context.rootObject.pushBack(data);
        if (SaveFile(file, context))
            return true;

        file->discardContent();
    }

    TRACE_ERROR("Failed to save '{}'", path);
    return false;
}

void ImportSaverThread::processSavingThread()
{
    TRACE_INFO("Cooker saving thread started");

    ScopeTimer timer;
    uint32_t numFilesSaved = 0;
    uint64_t numBytesWritten = 0;

    while (!m_saveThreadRequestExit.exchange(0))
    {
        // wait for file
        m_saveThreadSemaphore.wait(100);

        // get a file from list
        StringBuf path;
        ResourcePtr ptr;
        {
            auto lock = CreateLock(m_saveQueueLock);
            if (m_saveJobQueue.empty())
                continue;

            auto* job = m_saveJobQueue.top();
            ptr = job->unsavedResource;
            path = job->depotPath;
            m_saveCurrentResource = job->unsavedResource;
            m_saveJobQueue.pop();
            delete job;
        }

        // save to disk directly
        {
            ScopeTimer saveTimer;
            bool saved = saveSingleFile(ptr, path);
            TRACE_INFO("{} {} in {}", saved ? "Saved" : "Failed to save", path, saveTimer);
        }
                
        // unset guard
        {
            auto lock = CreateLock(m_saveQueueLock);
            m_saveCurrentResource = nullptr;
        }
    }

    TRACE_INFO("Cooker saved thread finished after {}, saved {} file(s), {}, there are {} outstanding jobs in queue", 
        timer, numFilesSaved, MemSize(numBytesWritten), m_saveJobQueue.size());
}

//--

END_BOOMER_NAMESPACE()
