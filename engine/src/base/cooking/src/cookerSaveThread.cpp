/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#include "build.h"
#include "cookerSaveThread.h"
#include "base/resources/include/resourceUncached.h"
#include "base/resources/include/resourceBinarySaver.h"
#include "base/object/include/nativeFileWriter.h"
#include "base/io/include/ioSystem.h"

namespace base
{
    namespace cooker
    {

        //--

        CookerSaveThread::CookerSaveThread()
            : m_saveThreadSemaphore(0, INT_MAX)
            , m_saveThreadRequestExit(0)
        {
            // start the saving thread
            ThreadSetup setup;
            setup.m_priority = ThreadPriority::AboveNormal;
            setup.m_name = "CookerSavingThread";
            setup.m_stackSize = 10 * 1024 * 1024; // saving requires large stack size
            setup.m_function = [this]() { processSavingThread(); };
            m_saveThread.init(setup);
        }

        CookerSaveThread::~CookerSaveThread()
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

        void CookerSaveThread::waitUntilDone()
        {
            base::ScopeTimer timer;

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
                base::Sleep(500);
            }

            if (printedMessage)
                TRACE_INFO("Saving finished in extra {}", timer);
        }

        bool CookerSaveThread::scheduleSave(const res::ResourcePtr& data, const io::AbsolutePath& path)
        {
            DEBUG_CHECK_EX(!path.empty(), "Invalid path");
            DEBUG_CHECK_EX(data, "Invalid data to save");

            // add to queue
            {
                auto saveLock = CreateLock(m_saveQueueLock);

                auto job = MemNew(SaveJob);
                job->absoultePath = path;
                job->unsavedResource = data;
                m_saveJobQueue.push(job);
            }

            // wakeup thread
            m_saveThreadSemaphore.release(1);
            return true;
        }

        bool CookerSaveThread::saveSingleFile(const res::ResourcePtr& data, const io::AbsolutePath& path)
        {
            // delete temp file, we keep them after failed save for inspection
            auto tempFilePath = path.addExtension(".out");
            IO::GetInstance().deleteFile(tempFilePath);

            // save to temp file - make sure the handle is closed before move (hence the scope)
            {
                auto tempWriter = IO::GetInstance().openForWriting(tempFilePath, false);
                if (!tempWriter)
                {
                    TRACE_ERROR("Failed to open '{}' for saving", tempFilePath);
                    return false;
                }

                // serialize
                auto binarySaver = base::CreateSharedPtr<res::binary::BinarySaver>();
                stream::NativeFileWriter tempFileWriter(tempWriter);
                stream::SavingContext savingContext(data);
                savingContext.m_contextName = base::TempString("{}", path);
                if (!binarySaver->saveObjects(tempFileWriter, savingContext))
                {
                    TRACE_ERROR("Failed to save '{}'", tempFilePath);
                    return false;
                }
            }

            // delete output file and swap it with temp file
            IO::GetInstance().deleteFile(path);
            if (!IO::GetInstance().moveFile(tempFilePath, path))
            {
                TRACE_ERROR("Failed to move '{}' into place after saving", tempFilePath);
                return false;
            }

            // saved
            return true;
        }

        void CookerSaveThread::processSavingThread()
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
                base::io::AbsolutePath path;
                base::res::ResourcePtr ptr;
                {
                    auto lock = CreateLock(m_saveQueueLock);
                    if (m_saveJobQueue.empty())
                        continue;

                    auto* job = m_saveJobQueue.top();
                    ptr = job->unsavedResource;
                    path = job->absoultePath;
                    m_saveCurrentResource = job->unsavedResource;
                    m_saveJobQueue.pop();
                    MemDelete(job);
                }

                // save to disk directly
                {
                    base::ScopeTimer saveTimer;
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
