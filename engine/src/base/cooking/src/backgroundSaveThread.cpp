/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#include "build.h"
#include "backgroundSaveThread.h"
#include "base/resources/include/resourceUncached.h"
#include "base/depot/include/depotStructure.h"

namespace base
{
    namespace cooker
    {

        //--

        BackgroundSaveThread::BackgroundSaveThread(depot::DepotStructure& depot)
            : m_saveThreadSemaphore(0, INT_MAX)
            , m_saveThreadRequestExit(0)
            , m_depot(depot)
        {
            // start the saving thread
            ThreadSetup setup;
            setup.m_priority = ThreadPriority::AboveNormal;
            setup.m_name = "BackgroundSavingThread";
            setup.m_stackSize = 10 * 1024 * 1024; // saving requires large stack size
            setup.m_function = [this]() { processSavingThread(); };
            m_saveThread.init(setup);
        }

        BackgroundSaveThread::~BackgroundSaveThread()
        {
            m_saveThreadRequestExit.exchange(1);

            // cancel all saving jobs
            {
                auto saveLock = CreateLock(m_saveQueueLock);
                for (auto* job : m_saveJobMap.values())
                    job->canceled = true;
            }

            // wait for thread to finish
            {
                ScopeTimer timer;
                m_saveThreadRequestExit.exchange(1);
                m_saveThread.close();
                TRACE_INFO("Cooker saving thread closed in '{}'", timer);
            }

            // cleanup
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

        bool BackgroundSaveThread::scheduleSave(const StringBuf& depotPath, const res::ResourcePtr& data)
        {
            DEBUG_CHECK_EX(!depotPath.empty(), "Invalid depot path");
            DEBUG_CHECK_EX(data, "Invalid data to save");

            if (depotPath && data)
            {
                // get mount point and add to queue if valid
                res::ResourceMountPoint mountPoint;
                if (m_depot.queryFileMountPoint(depotPath, mountPoint))
                {
                    // add to queue
                    {
                        auto saveLock = CreateLock(m_saveQueueLock);

                        // cancel existing job
                        {
                            SaveJob* job = nullptr;
                            if (m_saveJobMap.find(depotPath, job))
                                job->canceled = true;
                        }

                        // create new saving job
                        auto job = MemNew(SaveJob);
                        job->depotPath = depotPath;
                        job->canceled = false;
                        job->unsavedResource = data;
                        job->mountPoint = mountPoint;
                        m_saveJobMap[depotPath] = job;
                        m_saveJobQueue.push(job);
                    }

                    // wakeup thread
                    m_saveThreadSemaphore.release(1);
                    return true;
                }
                else
                {
                    TRACE_ERROR("Unknown depot mount for '{}'", depotPath);
                }
            }

            return false;
        }

        void BackgroundSaveThread::processSavingThread()
        {
            TRACE_INFO("Saving thread started");

            ScopeTimer timer;
            uint32_t numFilesSaved = 0;
            uint64_t numBytesWritten = 0;

            while (!m_saveThreadRequestExit.exchange(0))
            {
                // wait for file
                m_saveThreadSemaphore.wait(100);

                // get a file from list
                SaveJob* job = nullptr;
                {
                    auto lock = CreateLock(m_saveQueueLock);
                    if (!m_saveJobQueue.empty())
                    {
                        job = m_saveJobQueue.top();
                        m_saveJobQueue.pop();

                        if (m_saveJobMap[job->depotPath] == job)
                            m_saveJobMap.remove(job->depotPath);
                    }
                }

                // nothing to process, go back to waiting
                if (!job)
                    continue;

                // job was canceled
                if (job->canceled)
                {
                    TRACE_INFO("Skipping over canceled save job for '{}'", job->depotPath);
                    MemDelete(job);
                    continue;
                }

                // save to memory
                base::ScopeTimer serializeTimer;
                auto data = base::res::SaveUncachedToBuffer(job->unsavedResource, job->mountPoint);
                if (!data)
                {
                    TRACE_ERROR("Failed to save content of '{}' to in-memory buffer. There may be serialization issues.", job->depotPath);
                    MemDelete(job);
                    continue;
                }
                TRACE_INFO("Serialized content of '{}' in {}", job->depotPath, TimeInterval(serializeTimer.timeElapsed()));

                // save to depot
                base::ScopeTimer saveTimer;
                if (!m_depot.storeFileContent(job->depotPath, data))
                {
                    TRACE_ERROR("Failed to store content of '{}' to depot", job->depotPath);
                    MemDelete(job);
                    continue;
                }

                // saved
                TRACE_INFO("Saved content of '{}' in {}, total time {}", job->depotPath, TimeInterval(saveTimer.timeElapsed()), TimeInterval(serializeTimer.timeElapsed()));
                MemDelete(job);

                // stats
                numFilesSaved += 1;
                numBytesWritten += data.size();
            }

            TRACE_INFO("Cooker saved thread finished after {}, saved {} file(s), {}", timer, numFilesSaved, MemSize(numBytesWritten));
        }

        //--

    } // depot
} // base
