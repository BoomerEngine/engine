/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl\null #]
***/

#pragma once

#include "fiberSystem.h"
#include "fiberSystemImpl.h"

#include "base/system/include/spinLock.h"
#include "base/system/include/event.h"
#include "base/system/include/thread.h"
#include "base/containers/include/hashMap.h"

namespace base
{
    namespace fibers
    {
        namespace prv
        {

            /// thread based scheduler for the fibers
            /// each scheduled fiber gets a thread from a thread pool
            /// NOTE: there may be many more threads that cores on a given system because of the blocking nature of the fake fiber synchronization model in here
            /// NOTE: this is a fallback implementation and should not be used in final product (unless for specific purposes)
            class ThreadBasedScheduler : public IFiberScheduler
            {
            public:
                ThreadBasedScheduler();
                virtual ~ThreadBasedScheduler();

                /// IFiberScheduler
                virtual bool initialize(const IBaseCommandLine &commandline) override final;
                virtual void flush() override final;
                virtual void runSyncJobs();
                virtual void scheduleSync(const Job &job) override final;
                virtual void scheduleFiber(const Job &job, uint32_t numInvokations, bool child) override final;
                virtual WaitCounter createCounter(const char* userName, uint32_t count = 1) override final;
                virtual bool checkCounter(const WaitCounter& counter) override final;
                virtual void signalCounter(const WaitCounter &counter, uint32_t count = 1) override final;
                virtual void waitForCounterAndRelease(const WaitCounter &counter) override final;
                virtual void waitForMultipleCountersAndRelease(const WaitCounter* counters, uint32_t count) override final;
                virtual void yieldCurrentJob() override final;
                virtual bool isMainThread() const override final;
                virtual bool isMainFiber() const override final;
                virtual uint32_t workerThreadCount() const override final;
                virtual JobID currentJobID() const override final;

            private:
                //--

                struct WaitCounterData
                {
                    std::atomic<WaitCounterSeqID> m_seqId;
                    std::atomic<int> m_refCount;
                    std::atomic<int> m_count;
                    Event m_event;

                    INLINE WaitCounterData()
                        : m_seqId(0)
                        , m_refCount(0)
                        , m_count(0)
                        , m_event(true) // manual reset event
                    {}
                };

                struct JobPayload : public NoCopy
                {
                    RTTI_DECLARE_POOL(POOL_FIBERS)

                public:
                    TJobFunc m_funcToRun;
                    uint32_t m_invocationIndex;
                    const char* m_name;
                    JobID m_jobId;

                    JobPayload();
                };

                struct JobThread : public NoCopy
                {
                    RTTI_DECLARE_POOL(POOL_FIBERS)

                public:
                    Thread m_thread;
                    Event m_waitForJob;
                    uint32_t m_id;
                    JobPayload* m_job;

                    JobThread(ThreadBasedScheduler* owner, uint32_t id);
                    ~JobThread();
                };

                //---

                // allocate a thread for processing a job
                JobThread* allocThreadFromPool();

                // return thread to free list
                bool returnThreadToPool(JobThread* thread);

                //---

                // return counter to pool
                void returnCounterToPool(WaitCounterID id);

                //---

                // thread func
                void processJobs(JobThread* thread);

                // try to start first job from the pending list
                bool tryStartPendingJob();

                // try to start a specific job, returns true if it worked and thread was assigned to the job
                bool tryStartJob(JobPayload* payload);

                //---

                std::atomic<uint32_t> m_exiting;

                //---

                ThreadID m_mainThreadID;

                Array<JobThread*> m_freeThreads;
                Array<JobThread*> m_allThreads;
                Mutex m_freeThreadsLock;

                std::atomic<uint32_t> m_numThreadsRunning;
                std::atomic<uint32_t> m_numThreadsWaiting;

                //--

                std::atomic<uint32_t> m_nextCounterSeqId;

                Array<WaitCounterID> m_freeCounterIds;
                Mutex m_freeCountersLock;

                WaitCounterData* m_counters;
                Mutex m_countersLock;

                //--

                std::atomic<uint32_t> m_nextJobId;

                Array<JobPayload*> m_pendingJobs;
                Mutex m_pendingJobsLock;

                //--

                static TYPE_TLS JobPayload* CurrentJob;
            };

        } // prv
    } // fibers
} // base
