 /***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl #]
***/

#include "build.h"
#include "base/system/include/scopeLock.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/system/include/thread.h"
#include "fiberSystemCommon.h"
#include "base/containers/include/inplaceArray.h"

void DumpFibers();
void UnlockCounterManual(uint32_t id);

#ifdef PLATFORM_GCC
    #pragma GCC push_options
    #pragma GCC optimize ("O0")
#else
    #pragma optimize("",off)
#endif

BEGIN_BOOMER_NAMESPACE(base::fibers)

namespace prv
{
    //---

    static BaseScheduler* GBaseScheduler = nullptr;

    BaseScheduler::BaseScheduler()
    {
        GBaseScheduler = this;
    }

    BaseScheduler::~BaseScheduler()
    {
        if (!m_threads.empty())
        {
            DumpFibers();
            UnlockCounterManual(0);
        }

        ASSERT_EX(m_threads.empty(), "All threads should be closed");
    }

    uint32_t BaseScheduler::determineWorkerThreadCount(const IBaseCommandLine& cmdLine)
    {
        // use all cores on the machine if possible, get the count
        uint32_t maxJobThreads = GetNumberOfCores() - 1;
        TRACE_SPAM("System contains {} cores", maxJobThreads);

        // override the number of threads
        uint32_t numJobThreads = maxJobThreads;
        if (cmdLine.hasParam("numThreads"))
        {
            auto ret = range_cast<uint32_t>(cmdLine.singleValueInt("numThreads", 1));
            TRACE_INFO("User override of thread count to {}", ret);
            numJobThreads = std::clamp<uint32_t>(ret, 1, maxJobThreads);
        }

        // single thread override
        if (cmdLine.hasParam("nothreads"))
        {
            TRACE_INFO("No additional threads will be created for jobs");
            numJobThreads = 0;
        }

        // stats
        TRACE_INFO("Using {} fiber threads", numJobThreads);
        return numJobThreads;
    }

    bool BaseScheduler::initialize(const IBaseCommandLine& cmdLine)
    {
        // name starting thread
        SetThreadName("MainThread");

        // create job queue
        m_fiberSequenceNumber = 1;
        m_pendingJobsQueue = IOrderedQueue::Create();
        m_mainThreadJobsQueue = IOrderedQueue::Create();

        // create the overflow functions
        m_fiberPool.refill = [this]()
        {
            auto state = new FiberState; // no leak reported
            state->scheduler = this;
            state->fiberHandle = createFiberState(MAX_STACK, state);
            ASSERT_EX(state->fiberHandle.valid(), "Failed to create additional fiber state");
            return state;
        };

        // determine how much threads we need
        // NOTE: this may be 0 - in that case we run on a single thread (but still with fibers)
        auto numThreads = determineWorkerThreadCount(cmdLine);

        // reserve space (since we don't want to realloc)
        m_threads.reserve(numThreads);

        // create threads
        for (uint32_t i = 0; i < numThreads; ++i)
        {
            // create worker thread
            auto &state = m_threads.emplaceBack();
            state.index = range_cast<uint8_t>(m_threads.lastValidIndex());
            state.scheduler = this;
            sprintf(state.name, "FiberThread%u", i);
            state.threadHandle = createWorkerThread(&state);
        }

        // reserve space in the lists
        m_waitConterPool.init(MAX_JOBS * 4);

        // if we wished to convert the main thread to the worker thread do it now
        convertMainThread();
        return true;
    }

    void BaseScheduler::flush()
    {
        while (m_numScheduledJobs.load() > 0)
            yieldCurrentJob();
    }

    WaitCounter BaseScheduler::createCounter(const char* userName, uint32_t count /*= 1*/)
    {
        if (!count)
            return WaitCounter();

        return m_waitConterPool.allocWaitCounter(userName, count);
    }

    void BaseScheduler::schedulePendingJob(PendingJob* pendingJob)
    {
        if (!pendingJob->isMainThreadJob)
        {
            DEBUG_CHECK(pendingJob->sequenceNumber != 0);
            m_pendingJobsQueue->push(pendingJob, pendingJob->sequenceNumber);
        }
        else
        {
            m_mainThreadJobsQueue->push(pendingJob, 0);
        }
    }

    void BaseScheduler::scheduleInternal(const Job& job, uint32_t numInvokations, bool child)
    {
        uint64_t fiberSequenceNumber = child ? currentJobSequenceId() : ++m_fiberSequenceNumber;
        if (fiberSequenceNumber == 0)
            fiberSequenceNumber = ++m_fiberSequenceNumber; // main thread is parent

        m_numScheduledJobs += numInvokations;
        for (uint32_t i = 0; i < numInvokations; ++i)
        {
            auto pendingJob  = m_pendingJobPool.alloc();
            ASSERT(pendingJob->fiber == nullptr);
            ASSERT(pendingJob->next == nullptr);
            ASSERT(pendingJob->isAllocated);
            ASSERT(pendingJob->jobId != 0);

            pendingJob->sequenceNumber = fiberSequenceNumber;
            pendingJob->invocation = i;
            pendingJob->waitList = nullptr;
            pendingJob->state = PendingJobState::Scheduled;
            pendingJob->job = job;

            schedulePendingJob(pendingJob);
        }
    }

    void BaseScheduler::scheduleFiber(const Job& job, uint32_t numInvokations /*= 1*/, bool child /*= false*/)
    {
        scheduleInternal(job, numInvokations, child);
    }

    void BaseScheduler::scheduleSync(const Job &job)
    {
        m_syncList.push(job);
    }

    void BaseScheduler::runSyncJobs()
    {
        m_syncList.run();
    }

    void BaseScheduler::yieldCurrentJob()
    {
        // get active thread
        auto currentThread = currentThreadState();
        ASSERT(currentThread != nullptr);

        // get active fiber on thread
        auto currentFiber = currentThread->attachedFiber.load();
        ASSERT(currentFiber != nullptr);
        ASSERT(currentFiber != &currentThread->idleFiber); // we cant yield the idle fiber

        // get the job on the fiber
        auto currentJob = currentFiber->currentJob.load();
        ASSERT(currentJob != nullptr);
        ASSERT(currentJob->fiber == currentFiber);

        // remove local sink from the thread
        currentJob->logSink = logging::Log::MountLocalSink(nullptr);

        // capture current state of profiling blocks
        auto profilingBlocks = base::profiler::Block::CaptureAndYield();
        auto taskBlocks = base::Task::DetachTask();

        // tell the scheduler to yield the job
        FATAL_CHECK(PendingJobState::Running == currentJob->state.exchange(PendingJobState::Yielded));
        FATAL_CHECK(!currentThread->jobToReschedule.exchange(currentJob))

        // switch back to the scheduler
        FATAL_CHECK(currentThread->attachedFiber.exchange(nullptr));
        FATAL_CHECK(true == currentFiber->isRunning.exchange(false));
        switchFiber(currentFiber->fiberHandle, currentThread->idleFiber.fiberHandle);

        ///--------------- thread boundary (we may come back with a different thread)

        currentThread = currentThreadState();
        ASSERT(currentThread != nullptr);

        // attach back to the new thread (note: it may be different than the one we departed with)
        FATAL_CHECK(!currentThread->attachedFiber.exchange(currentFiber));
        currentJob->lastThreadIndex = currentThread->index;
        FATAL_CHECK(false == currentFiber->isRunning.exchange(true));
        FATAL_CHECK(PendingJobState::Running == currentJob->state.load());

        // restore local sink from the thread
        auto prevLogSink = logging::Log::MountLocalSink(currentJob->logSink);
        ASSERT(prevLogSink == nullptr);

        // resume profiling
        base::profiler::Block::Resume(profilingBlocks);
        base::Task::AttachTask(taskBlocks);
    }

    bool BaseScheduler::isMainThread() const
    {
        auto currentThread  = currentThreadState();
        return currentThread == &m_mainThreadState;
    }

    bool BaseScheduler::isMainFiber() const
    {
        auto currentThread  = currentThreadState();
        if (currentThread)
            return currentThread->attachedFiber == &m_mainFiberState;

        return false;
    }

    uint32_t BaseScheduler::workerThreadCount() const
    {
        return m_threads.size();
    }

    uint64_t BaseScheduler::currentJobSequenceId() const
    {
        if (auto currentThread = currentThreadState())
            if (auto currentFiber = currentThread->attachedFiber.load())
                if (auto currentJob = currentFiber->currentJob.load())
                    return currentJob->sequenceNumber;

        return 0;
    }

    JobID BaseScheduler::currentJobID() const
    {
        if (auto currentThread  = currentThreadState())
            if (auto currentFiber  = currentThread->attachedFiber.load())
                if (auto currentJob = currentFiber->currentJob.load())
                    return currentJob->jobId;

        return 0;
    }

    bool BaseScheduler::checkCounter(const WaitCounter& counter)
    {
        return m_waitConterPool.checkWaitCounter(counter);
    }

    void BaseScheduler::waitForMultipleCountersAndRelease(const WaitCounter* counters, uint32_t numCounters)
    {
        // collect counters that were not yet signaled
        base::InplaceArray<WaitCounter, 15> tempCounters;
        tempCounters.reserve(numCounters);
        for (uint32_t i = 0; i < numCounters; ++i)
        {
            auto counter = counters[i];
            if (!checkCounter(counter))
                tempCounters.pushBack(counter);
        }

        // single thing to wait for, use directly
        if (1 == tempCounters.size())
        {
            waitForCounterAndRelease(tempCounters[1]);
        }
        else if (2 >= tempCounters.size())
        {
            // TODO: better implementation!

            // create waiting job
            Job job;
            auto finalCounter = createCounter("MergedCounter", 1);
            job.func = [this, &tempCounters, finalCounter](FIBER_FUNC)
            {
                for (auto& counter : tempCounters)
                    waitForCounterAndRelease(counter);
                signalCounter(finalCounter);
            };
            job.name = "WaitForMergedCounter";

            scheduleFiber(job, 1, true);
            waitForCounterAndRelease(finalCounter);
        }
    }

    void BaseScheduler::waitForCounterAndRelease(const WaitCounter& counter)
    {
        // nothing to wait on
        if (counter.empty())
            return;

        // get the thread we are operating from
        auto currentThread = currentThreadState();
        if (currentThread == nullptr)
        {
            // not a fiber thread, do a standard wait
            m_waitConterPool.waitForCounterThreadEvent(counter);
            return;
        }

        // get the fiber running on this thread
        auto currentFiber  = currentThread->attachedFiber.load();
        ASSERT(currentFiber != nullptr);

        // get current job
        auto currentJob  = currentFiber->currentJob.load();
        ASSERT(currentJob != nullptr);
        ASSERT(currentJob->fiber == currentFiber);

        // remove local sink from the thread
        currentJob->logSink = logging::Log::MountLocalSink(nullptr);

        // enter the wait state
        auto profilingBlocks  = base::profiler::Block::CaptureAndYield();
        auto taskBlocks  = base::Task::DetachTask();

        // switch
        FATAL_CHECK(true == currentFiber->isRunning.exchange(false));
        FATAL_CHECK(currentFiber == currentThread->attachedFiber.exchange(nullptr));
        auto prevState = currentJob->state.exchange(PendingJobState::Waiting);
        FATAL_CHECK(PendingJobState::Running == prevState);

        // tell the scheduler to insert this job into the wait list
        currentJob->waitForCounter = counter;
        FATAL_CHECK(!currentThread->jobToReschedule.exchange(currentJob));

        // switch back to the scheduler
        switchFiber(currentFiber->fiberHandle, currentThread->idleFiber.fiberHandle);

        ///--------------- thread boundary (we may come back with a different thread)

        currentThread = currentThreadState();

        // restore from wait state
        ASSERT(currentJob->waitForCounter.empty());
        FATAL_CHECK(false == currentFiber->isRunning.exchange(true));
        FATAL_CHECK(!currentThread->attachedFiber.exchange(currentFiber));
        FATAL_CHECK(PendingJobState::Running == currentJob->state.load());
        currentJob->lastThreadIndex = currentThread->index;

        // restore local sink from the thread
        auto prevLogSink = logging::Log::MountLocalSink(currentJob->logSink);
        ASSERT(prevLogSink == nullptr);

        // resume profiling
        base::profiler::Block::Resume(profilingBlocks);
        base::Task::AttachTask(taskBlocks);
    }

    void BaseScheduler::signalCounter(const WaitCounter& counter, uint32_t count /*= 1*/)
    {
        // get the wait list to release
        auto jobToReschedule  = m_waitConterPool.signalWaitCounter(counter, count);
        while (jobToReschedule != nullptr)
        {
            auto nextJob  = jobToReschedule->next;
            jobToReschedule->next = nullptr;

            // change job state
            auto prevState = jobToReschedule->state.exchange(PendingJobState::Yielded);
            FATAL_CHECK(PendingJobState::Waiting == prevState);
            ASSERT(jobToReschedule->waitList != nullptr);
            jobToReschedule->waitList = nullptr;
#ifndef BUILD_FINAL
            jobToReschedule->waitCallstack.reset();
#endif

            // get back to the unlocked job as soon as possible
            schedulePendingJob(jobToReschedule);
            jobToReschedule = nextJob;
        }
    }

    void BaseScheduler::cleanupPendingJob(PendingJob* job)
    {
        ASSERT(job->fiber == nullptr);
        ASSERT(job->state.load() == PendingJobState::Finished);

        // release the pending job object first
        ASSERT(!job->next);
        job->fiber = nullptr;
        job->invocation = 0;
        job->lastThreadIndex = 255;
        job->job.func = TJobFunc();
        job->waitList = nullptr;
#ifndef BUILD_FINAL
        job->waitCallstack.reset();
#endif
        job->state = PendingJobState::Free;

        // release back
        m_pendingJobPool.release(job);

        // update global count of running jobs
        --m_numScheduledJobs;
    }

//#define MEASURE_JOB_STACKS

    void BaseScheduler::processFiberJob(FiberState* state)
    {
        // make sure any updates were written
        ASSERT(state->currentJob.load() != nullptr);

        // measure stack size
#ifdef MEASURE_JOB_STACKS
        auto stackCur  = (uint8_t*)alloca(1);
        if (state->fiberHandle.stackBase != nullptr)
        {
            ASSERT(stackCur >= state->fiberHandle.stackBase && stackCur <= state->fiberHandle.stackEnd);

            // clear stack
            auto ptr  = state->fiberHandle.stackBase;
            while (ptr + 4 < stackCur)
            {
                *(uint32_t*)ptr++ = 0xBAADF00D;
                ptr += 4;
            }
        }
#endif

        // execute job
        {
            auto job = state->currentJob.load();
            ASSERT(job->waitList == nullptr);
            ASSERT(job->isAllocated);

            // switch state
            FATAL_CHECK(PendingJobState::Scheduled == job->state.exchange(PendingJobState::Running));

            // run job
            FATAL_CHECK(false == state->isRunning.exchange(true));
            job->job.func(job->invocation);
            job->job.func = TJobFunc();
            FATAL_CHECK(true == state->isRunning.exchange(false));

            // finalize state
            FATAL_CHECK(PendingJobState::Running == job->state.exchange(PendingJobState::Finished));
        }

#ifdef MEASURE_JOB_STACKS
        // count used stack space
        if (state->fiberHandle.stackBase != nullptr)
        {
            auto ptr  = state->fiberHandle.stackBase;
            while (ptr + 4 < stackCur)
            {
                if (*(uint32_t*)ptr++ != 0xBAADF00D)
                    break;
                ptr += 4;
            }

            auto stackSize = stackCur - ptr;
            TRACE_INFO("Job '{}', stack {}", rawJob->name, stackSize);
        }
#endif
    }

    void BaseScheduler::fiberFunc(FiberState* state)
    {
        // intro
        FATAL_CHECK(!currentThreadState()->attachedFiber.exchange(state));

        // endless loop
        for (;;) // we never exit
        {
            // process the job
            processFiberJob(state);

            // we finished the job and we can release the fiber, we can't do it here yet only the part we are calling can do it
            auto threadState = currentThreadState();
            FATAL_CHECK(!threadState->fiberToRelease.exchange(state));

            // restore local sink from the thread
            auto prevLogSink = logging::Log::MountLocalSink(nullptr);
            ASSERT(prevLogSink == nullptr);

            // jump back to the original threads scheduler
            auto prevState = threadState->attachedFiber.exchange(nullptr);
            ASSERT(state == prevState);
            switchFiber(state->fiberHandle, threadState->idleFiber.fiberHandle);

            ///--------------- thread boundary (we may come back with a different thread)

            // restore local sink from the thread
            auto prevLogSink2 = logging::Log::MountLocalSink(nullptr);
            ASSERT(prevLogSink2 == nullptr);

            // attach fiber to the thread we came back with (it may be different)
            threadState = currentThreadState();
            FATAL_CHECK(!threadState->attachedFiber.exchange(state));
        }
    }

    void BaseScheduler::schedulerFunc()
    {
        auto originalThread  = currentThreadState();
        auto needsFirstJob = !originalThread->isMainThread;

        // when entering scheduler for the main thread there will be no jobs pending
        while (1)
        {
            auto currentThread  = currentThreadState();
            ASSERT(currentThread != nullptr);
            ASSERT(currentThread == originalThread);

            // the main thread when it enters the scheduler already has main job
            if (needsFirstJob)
            {
                // wait for the pending list to become non-empty
                // we periodically check the exit flag was not risen
                auto job = (PendingJob *)(currentThread->isMainThread ? m_mainThreadJobsQueue->pop() : m_pendingJobsQueue->pop());
                if (!job)
                {
                    //TRACE_WARNING("Got NULL job on thread '{}', assuming we are exiting", currentThread->name);
                    break;
                }

                // if the acquired job has no fiber assigned do it now
                auto jobState = job->state.load();
                if (job->fiber == nullptr)
                {
                    ASSERT_EX(job->state.load() == PendingJobState::Scheduled, "Invalid state of popped job");
                    job->fiber = m_fiberPool.allocFiber();
                    ASSERT(job->fiber != nullptr);
                    ASSERT(job->fiber->currentJob.load() == nullptr);
                    job->fiber->currentJob = job;
                    job->logSink = job->job.localSink; // will be assigned when we get on a proper thread
                }
                else
                {
                    auto prevState = job->state.exchange(PendingJobState::Running);
                    FATAL_CHECK(PendingJobState::Yielded == prevState);
                }

                // update thread information
                job->lastThreadIndex = currentThread->index;

                // switch to the fiber code
                FATAL_CHECK(&currentThread->idleFiber == currentThread->attachedFiber.exchange(nullptr));
                switchFiber(currentThread->idleFiber.fiberHandle, job->fiber->fiberHandle);
            }
            else
            {
                needsFirstJob = true; // from now on we are inside the loop and we should pick the job
            }

            //--------------- thread boundary

            ASSERT(currentThreadState() == currentThread);
            FATAL_CHECK(!currentThread->attachedFiber.exchange(&currentThread->idleFiber));

            // do we have a fiber to release ?
            if (auto fiberToRelease  = currentThread->fiberToRelease.exchange(nullptr))
            {
                ASSERT(!fiberToRelease->isRunning);
                ASSERT(!fiberToRelease->isMainThreadFiber);

                // unlink job and fiber that were linked together
                auto jobToRelease  = fiberToRelease->currentJob.exchange(nullptr);
                ASSERT(jobToRelease->fiber == fiberToRelease);
                jobToRelease->fiber = nullptr;

                // we've finished the top-level job, release it
                cleanupPendingJob(jobToRelease);

                // free the fiber
                m_fiberPool.releaseFiber(fiberToRelease);
            }

            // reschedule another job
            if (auto jobToSchedule  = currentThread->jobToReschedule.exchange(nullptr))
            {
                auto counter = jobToSchedule->waitForCounter;
                jobToSchedule->waitForCounter = WaitCounter();

                // if we have to wait for a specific counter before we can restart the job
                // than try to insert into the wait queue, if this fails it means that the job has already finished and we should come back
                if (!counter.empty())
                {
                    if (!m_waitConterPool.addJobToWaitingList(counter, jobToSchedule))
                    {
                        // no counter, schedule immediately
                        FATAL_CHECK(PendingJobState::Waiting == jobToSchedule->state.exchange(PendingJobState::Yielded));
                        schedulePendingJob(jobToSchedule);
                    }
                }
                else
                {
                    // we just yielded
                    schedulePendingJob(jobToSchedule);
                }
            }
        }
    }
            
    static const char* GetStatusText(uint8_t status)
    {
        switch (status)
        {
            case 0: return "Free";
            case 1: return "Scheduled";
            case 2: return "Running";
            case 3: return "Yielded";
            case 4: return "Waiting";
            case 5: return "Finished";
        }

        return "Unknown";
    }

    void BaseScheduler::manualCounterUnlock(uint32_t id)
    {
        auto counter = m_waitConterPool.findCounter(id);
        if (!counter.empty())
            signalCounter(counter);
    }

    void BaseScheduler::dump()
    {
        uint32_t pendingJobCounter = 0;

        // dump main thread
        {
            fprintf(stderr, "MainThread\n");

            if (auto f  = m_mainThreadState.attachedFiber.load())
            {
                if (f == &m_mainFiberState)
                {
                    fprintf(stderr, "   Running Main Fiber");
                }
                else
                {
                    fprintf(stderr, "   Running Other Fiber ");

                    if (auto j = f->currentJob.load())
                    {
                        fprintf(stderr, "%u, %s, LTI %u, %s",
                                j->jobId, j->job.name, j->lastThreadIndex, j->isMainThreadJob ? "MainThreadJob" : "");
                    }
                }
            }
            else
            {
                fprintf(stderr, "   IDLE");
            }

            if (auto jobToReschedule  = m_mainThreadState.jobToReschedule.load())
            {
                fprintf(stderr, ": Reschedule(ID %u)", jobToReschedule->jobId);

                if (!jobToReschedule->waitForCounter.empty())
                {
                    fprintf(stderr, " with counter ID %u, SEQID %u", jobToReschedule->waitForCounter.id(), jobToReschedule->waitForCounter.sequenceId());
                }
            }

            fprintf(stderr, "   \n");

            if (m_mainJobState.waitList != nullptr)
            {
                fprintf(stderr, "   Main thread waiting for counter ID %u, SEQ %u: ", m_mainJobState.waitList->localId, m_mainJobState.waitList->seqId);
                m_waitConterPool.printCounterInfo(*m_mainJobState.waitList);
                fprintf(stderr, "   \n");
            }

            fprintf(stderr, "   \n");
        }

        // dump threads
        {
            fprintf(stderr, "Threads (%u total):\n", m_threads.size());

            for (auto& threadInfo : m_threads)
            {
                fprintf(stderr, "   [%u]: %s ", threadInfo.index, threadInfo.name);

                auto f  = threadInfo.attachedFiber.load();
                if (f != nullptr)
                {
                    if (f == &threadInfo.idleFiber)
                    {
                        fprintf(stderr, ": In scheduler");
                    }
                    else
                    {
                        fprintf(stderr, ": Running Fiber ");
                        if (auto j = f->currentJob.load())
                        {
                            fprintf(stderr, "%u, %s, LTI %u, %s",
                                    j->jobId, j->job.name, j->lastThreadIndex, j->isMainThreadJob ? "MainThreadJob" : "");
                        }
                    }
                }
                else
                {
                    fprintf(stderr, ": No fiber");
                }

                if (auto jobToReschedule  = threadInfo.jobToReschedule.load())
                {
                    fprintf(stderr, ": Reschedule(ID %u)", jobToReschedule->jobId);

                    if (!jobToReschedule->waitForCounter.empty())
                    {
                        fprintf(stderr, " with counter ID %u, SEQID %u", jobToReschedule->waitForCounter.id(), jobToReschedule->waitForCounter.sequenceId());
                    }
                }

                fprintf(stderr, "\n");
            }
            fprintf(stderr, "\n");
        }

        // dump running fibers
        {
            fprintf(stderr, "Active fibers:\n");

            uint32_t count = 0;
            m_fiberPool.inspect([this, &count](const FiberState* state)
            {
                fprintf(stderr, "   [%u]: 0x%08llX", count, (uint64_t)state);

                auto job  = state->currentJob.load();
                if (job)
                {
                    fprintf(stderr, ", ID%u: '%s', %s, LTI %u %s",
                            job->jobId, job->job.name,
                            GetStatusText((uint8_t)job->state.load()),
                            job->lastThreadIndex, job->isMainThreadJob ? "MainThreadJob" : "");
                    ASSERT(job->fiber == state);

                    if (job->waitList != nullptr)
                    {
                        fprintf(stderr, ", Waiting for ID %u, SEQ %u (", job->waitList->localId, job->waitList->seqId);
                        m_waitConterPool.printCounterInfo(*job->waitList);
                        fprintf(stderr, ")");
                    }
                }

                fprintf(stderr, "\n");
                count += 1;
            });

            if (count == 0)
                fprintf(stderr, "  No active fibers\n\n");
            else
                fprintf(stderr, "  %u active fibers\n\n", count);
        }

        // dump wait lists
        {
            fprintf(stderr, "Active wait lists:\n");
            uint32_t count = 0;
            m_waitConterPool.inspect([this, &count](const WaitList* state)
                                {
                                    fprintf(stderr, "   [%u]: ID %u, SEQID %u, ", count, state->localId, state->seqId);
                                    m_waitConterPool.printCounterInfo(*state);
                                    fprintf(stderr, "\n");
                                    count += 1;
                                });

            if (count == 0)
                fprintf(stderr, "  No active wait lists\n\n");
            else
                fprintf(stderr, "  %u wait lists\n\n", count);
        }

        // dump pending jobs
        {
            fprintf(stderr, "Pending jobs:\n");
            m_pendingJobsQueue->inspect([this, &pendingJobCounter](QUEUE_INSPECTOR)
                                        {
                                            auto job  = (const PendingJob*)payload;
                                            fprintf(stderr, "   [%u]: ID%u: '%s', %s, LTI %u %s",
                                                    pendingJobCounter, job->jobId, job->job.name,
                                                    GetStatusText((uint8_t)job->state.load()), 
                                                    job->lastThreadIndex, job->isMainThreadJob ? "MainThreadJob" : "");
                                            //ASSERT(job->fiber == nullptr);
                                            ASSERT(job->waitList == nullptr);

                                            fprintf(stderr, "\n");
                                            pendingJobCounter += 1;
                                        });

            if (pendingJobCounter == 0)
                fprintf(stderr, "  No pending jobs\n\n");
            else
                fprintf(stderr, "  %u pending jobs total\n\n", pendingJobCounter);
        }

        // validate main thread state
        ASSERT(m_mainJobState.isMainThreadJob);
        ASSERT(m_mainFiberState.currentJob == &m_mainJobState);
        ASSERT(m_mainJobState.fiber == &m_mainFiberState);

        // validate some extra shit
        m_waitConterPool.validate();
        m_fiberPool.validate();
    }

} // prv

END_BOOMER_NAMESPACE(base::fibers)

void DumpFibers()
{
     if (base::fibers::prv::GBaseScheduler)
         base::fibers::prv::GBaseScheduler->dump();
}

void UnlockCounterManual(uint32_t id)
{
    if (base::fibers::prv::GBaseScheduler)
        base::fibers::prv::GBaseScheduler->manualCounterUnlock(id);
}

#ifdef PLATFORM_GCC
    #pragma GCC pop_options
#else
    #pragma optimize("",on)
#endif