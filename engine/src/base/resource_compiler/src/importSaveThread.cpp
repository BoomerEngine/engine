/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importSaveThread.h"
#include "base/resource/include/resourceUncached.h"
#include "base/resource/include/resourceBinarySaver.h"
#include "base/system/include/thread.h"
#include "base/resource_compiler/include/depotStructure.h"
#include "base/object/include/memoryWriter.h"

namespace base
{
    namespace res
    {

        //--

        ImportSaverThread::ImportSaverThread(depot::DepotStructure& depot)
            : m_saveThreadSemaphore(0, INT_MAX)
            , m_saveThreadRequestExit(0)
            , m_depot(depot)
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
                    MemDelete(job);
                }
            }
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

                auto job = MemNew(SaveJob);
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
            // setup memory writer
            stream::MemoryWriter tempFileWriter;
            stream::SavingContext savingContext(data);
            savingContext.m_contextName = path;

            // serialize content using binary writer to a memory buffer
            auto binarySaver = CreateSharedPtr<binary::BinarySaver>();
            if (!binarySaver->saveObjects(tempFileWriter, savingContext))
            {
                TRACE_ERROR("Failed to serialize '{}'", path);
                return false;
            }

            // store content in file
            return m_depot.storeFileContent(path, tempFileWriter.extractData());
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
                    MemDelete(job);
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

    } // depot
} // base
