/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl\null #]
***/

#include "build.h"
#include "fiberSystemThreadBased.h"

#include "base/system/include/thread.h"
#include "base/system/include/scopeLock.h"

namespace base
{
    namespace fibers
    {
        namespace prv
        {

            TYPE_TLS ThreadBasedScheduler::JobPayload* ThreadBasedScheduler::CurrentJob = 0;

            ThreadBasedScheduler::ThreadBasedScheduler()
                : m_nextCounterSeqId(1)
                , m_nextJobId(1)
                , m_numThreadsWaiting(0)
                , m_numThreadsRunning(0)
            {
                // create the counter
                auto maxCounter = 65535;
                m_counters = new WaitCounterData[maxCounter];
                m_freeCounterIds.reserve(maxCounter);
                for (int i=(int)maxCounter-1; i>=0; --i)
                    m_freeCounterIds.pushBack((WaitCounterID)i);
            }

            ThreadBasedScheduler::~ThreadBasedScheduler()
            {
                // set the exit flag
                m_exiting = 1;

                // get all threads
                m_freeThreadsLock.acquire();
                auto allThreads = std::move(m_allThreads);
                for (auto thread  : allThreads)
                    thread->m_waitForJob.trigger();
                m_freeThreads.clear();
                m_freeThreadsLock.release();

                // free counters
                m_freeCounterIds.clear();
                delete [] m_counters;
                m_counters = nullptr;

                // close them
                allThreads.clearPtr();
            }
            
            bool ThreadBasedScheduler::initialize(const IBaseCommandLine &commandline)
            {
                m_mainThreadID = GetCurrentThreadID();
                TRACE_SPAM("Main thread ID={}", m_mainThreadID);
                return true;
            }

            void ThreadBasedScheduler::flush()
            {
                // TODO!
            }

            WaitCounter ThreadBasedScheduler::createCounter(const char* userName, uint32_t count /*= 1*/)
            {
                // get the free id
                m_freeCountersLock.acquire();
                ASSERT_EX(!m_freeCounterIds.empty(), "To many active counters");
                auto id = m_freeCounterIds.back();
                m_freeCounterIds.popBack();
                m_freeCountersLock.release();

                // allocate sequence id
                auto seqId = ++m_nextCounterSeqId;

                // setup counter
                m_countersLock.acquire();
                auto& counter = m_counters[id];
                ASSERT_EX(counter.m_seqId.load() == 0, "Allocated counter is in use");
                ASSERT_EX(counter.m_count.load() == 0, "Invalid initial state of counter");
                ASSERT_EX(counter.m_refCount.load() == 0, "Invalid initial state of counter");
                counter.m_count = count;
                counter.m_refCount = 1; // we hold the reference until we call signal, NOTE: all created counters must be signaled
                counter.m_seqId = seqId;
                m_countersLock.release();

                // return wrapper
                TRACE_SPAM("Created counter {}, initial value {}", seqId, count);
                return WaitCounter(id, seqId);
            }

            void ThreadBasedScheduler::scheduleSync(const Job &job)
            {
                // TODO
            }

            void ThreadBasedScheduler::scheduleFiber(const Job& job, uint32_t numInvokations /*= 1*/, bool child)
            {
                ASSERT(job.func);
                ASSERT(numInvokations);

                // create jobs
                for (uint32_t i=0; i<numInvokations; ++i)
                {
                    // create payload
                    auto payload  = MemNew(JobPayload);
                    payload->m_invocationIndex = i;
                    payload->m_funcToRun = job.func;
                    payload->m_name = job.name;
                    payload->m_jobId = ++m_nextJobId;

                    // start
                    if (!tryStartJob(payload))
                    {
                        ScopeLock<Mutex> lock(m_pendingJobsLock);
                        m_pendingJobs.pushBack(payload);
                    }
                }
            }

            bool ThreadBasedScheduler::tryStartPendingJob()
            {
                ScopeLock<Mutex> lock(m_pendingJobsLock);
                if (!m_pendingJobs.empty())
                {
                    auto pendingJob  = m_pendingJobs[0];
                    if (tryStartJob(pendingJob))
                    {
                        m_pendingJobs.erase(0);
                        return true;
                    }
                }

                return false;
            }

            bool ThreadBasedScheduler::tryStartJob(JobPayload* payload)
            {
                // check if we can execute this job
                //auto maxThreadsRunnig = 1 + m_numThreadsWaiting.load();
                //if (m_numThreadsRunning.load() >= maxThreadsRunnig)
                    //return false;

                // allocate thread for execution
                auto thread  = allocThreadFromPool();
                if (!thread)
                    return false;

                // start job
                ASSERT(thread->m_job == nullptr);
                thread->m_job = payload;
                thread->m_waitForJob.trigger();
                return true;
            }

            bool ThreadBasedScheduler::isMainThread() const
            {
                return GetCurrentThreadID() == m_mainThreadID;
            }

            bool ThreadBasedScheduler::isMainFiber() const
            {
                return GetCurrentThreadID() == m_mainThreadID;
            }

            uint32_t ThreadBasedScheduler::workerThreadCount() const
            {
                return 4;
            }

            JobID ThreadBasedScheduler::currentJobID() const
            {
                return CurrentJob ? CurrentJob->m_jobId : 0;
            }

            void ThreadBasedScheduler::yieldCurrentJob()
            {
                Yield();
            }

            bool ThreadBasedScheduler::checkCounter(const WaitCounter& counter)
            {
                // invalid counter
                if (counter.empty())
                    return true;

                // get data
                auto& entry = m_counters[counter.id()];
                return entry.m_seqId.load() != counter.sequenceId(); // already signaled if removed
            }

            void ThreadBasedScheduler::runSyncJobs()
            {
                // TODO
            }

            void ThreadBasedScheduler::waitForMultipleCountersAndRelease(const WaitCounter* counters, uint32_t count)
            {
                // TODO
            }

            void ThreadBasedScheduler::waitForCounterAndRelease(const WaitCounter& counter)
            {
                // invalid counter
                if (counter.empty())
                    return;

                // get entry
                m_countersLock.acquire();
                auto& entry = m_counters[counter.id()];

                // wait for the entry only if of proper sequence
                if (entry.m_seqId.load() == counter.sequenceId())
                {
                    ++entry.m_refCount; // keep the entry (and the event alive)
                    m_countersLock.release();

                    // wait for the event
                    TRACE_SPAM("Wait for counter {}", counter.id());
                    ++m_numThreadsWaiting;
                    entry.m_event.waitInfinite();
                    --m_numThreadsWaiting;

                    // we are done waiting
                    TRACE_SPAM("Wait for counter {} finished", counter.id());

                    // release counter to the pool
                    if (0 == --entry.m_refCount)
                        returnCounterToPool(counter.id());
                }
                else
                {
                    m_countersLock.release();
                }
            }

            void ThreadBasedScheduler::signalCounter(const WaitCounter& counter, uint32_t count /*= 1*/)
            {
                ASSERT_EX(!counter.empty(), "Trying to signal invalid counter");

                m_countersLock.acquire();

                auto& entry = m_counters[counter.id()];
                if (entry.m_seqId.load() == counter.sequenceId())
                {
                    ASSERT_EX(entry.m_refCount.load() >= 1, "Counter is already dead");

                    auto newCount = entry.m_count -= count;
                    if (0 == newCount)
                    {
                        TRACE_SPAM("Counter {} finished", counter.id());

                        // signal waiting threads
                        entry.m_event.trigger();

                        // release reference (originally acquired in the createCounter)
                        if (0 == --entry.m_refCount)
                            returnCounterToPool(counter.id());
                    }
                }

                m_countersLock.release();
            }

            //--

            ThreadBasedScheduler::JobPayload::JobPayload()
                : m_invocationIndex(0)
                , m_jobId(0)
                , m_name("Job")
            {}

            ThreadBasedScheduler::JobThread::JobThread(ThreadBasedScheduler* owner, uint32_t id)
                : m_job(nullptr)
                , m_id(id)
            {
                ThreadSetup setup;
                setup.m_name = "FiberThread";
                setup.m_stackSize = 256 * 1024; // all big
                setup.m_function = [owner, this]() { owner->processJobs(this); };
                m_thread.init(setup);
            }

            ThreadBasedScheduler::JobThread::~JobThread()
            {
                m_thread.close();
            }

            void ThreadBasedScheduler::processJobs(JobThread* thread)
            {
                for (;;)
                {
                    // wait for being unblocked
                    TRACE_SPAM("Waiting for jobs on thread {}", thread->m_id);
                    thread->m_waitForJob.waitInfinite();

                    // handle exit condition
                    if (m_exiting.load())
                    {
                        TRACE_SPAM("Got request to close thread {}", thread->m_id);
                        break;
                    }

                    // bind the job to the thread
                    auto job = thread->m_job;
                    CurrentJob = thread->m_job;
                    ASSERT(thread->m_job != nullptr);
                    thread->m_job = nullptr;

                    // execute the job
                    ASSERT_EX(job->m_funcToRun, "Thread scheduled without proper job");
                    //TRACE_SPAM("Processing job {}, IID {}", job->m_counterToSingal.sequenceId(), job->m_invocationIndex);
                    {
                        PC_SCOPE_LVL1(FiberJob);
                        auto func = std::move(job->m_funcToRun);
                        func(job->m_invocationIndex);
                    }
                    //TRACE_SPAM("Finished job {}, IID {}", job->m_counterToSingal.sequenceId(), job->m_invocationIndex);

                    // cleanup
                    MemDelete(job);

                    // return thread to the pool
                    if (!returnThreadToPool(thread))
                        break;
                }
            }

            //--

            ThreadBasedScheduler::JobThread* ThreadBasedScheduler::allocThreadFromPool()
            {
                ScopeLock<Mutex> lock(m_freeThreadsLock);

                // thread pool was closed, do not allocate any new threads
                if (m_exiting.load())
                    return nullptr;

                // use existing thread if possible
                if (!m_freeThreads.empty())
                {
                    // pop thread
                    auto thread  = m_freeThreads.back();
                    m_freeThreads.popBack();

                    // setup
                    auto count = ++m_numThreadsRunning;
                    TRACE_SPAM("NumThreadsRunning++: {}", count);
                    return thread;
                }

                // create new thread
                auto thread = MemNew(JobThread, this, m_allThreads.size());
                m_allThreads.pushBack(thread);

                // stats
                auto count = ++m_numThreadsRunning;
                TRACE_SPAM("NumThreadsRunning++: {}", count);
                return thread;
            }

            bool ThreadBasedScheduler::returnThreadToPool(JobThread* thread)
            {
                ASSERT(thread->m_job == nullptr);

                m_freeThreadsLock.acquire();

                auto count = --m_numThreadsRunning;
                TRACE_SPAM("NumThreadsRunning--: {}", count);

                m_freeThreads.pushBack(thread);
                m_freeThreadsLock.release();

                tryStartPendingJob();
                return true; // keep alive
            }

            //---

            void ThreadBasedScheduler::returnCounterToPool(WaitCounterID id)
            {
                auto& entry = m_counters[id];
                ASSERT_EX(entry.m_refCount.load() == 0, "Counter is still in use");
                ASSERT_EX(entry.m_count.load() == 0, "Counter is still not singaled");
                ASSERT_EX(entry.m_seqId.load() != 0, "Counter was not properly allocated");

                // reset
                entry.m_event.reset();
                entry.m_seqId = 0;

                // return to free pool
                m_freeCountersLock.acquire();
                m_freeCounterIds.pushBack(id);
                m_freeCountersLock.release();
            }

        } // win
    } // fibers
} // base
