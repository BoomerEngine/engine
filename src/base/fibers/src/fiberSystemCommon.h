/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl #]
***/

#pragma once

#include "fiberSystem.h"
#include "fiberSystemImpl.h"
#include "base/containers/include/array.h"
#include "base/system/include/spinLock.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/system/include/debug.h"
#include "base/system/include/orderedQueue.h"

BEGIN_BOOMER_NAMESPACE(base::fibers)

namespace prv
{

    /// Common stuff for true fiber based scheduler
    class BaseScheduler : public IFiberScheduler
    {
    public:
        BaseScheduler();
        virtual ~BaseScheduler();

        // IFiberScheduler
        virtual bool initialize(const IBaseCommandLine& cmdLine) override final;
        virtual void runSyncJobs() override final;
        virtual void flush() override final;
        virtual void scheduleFiber(const Job &job, uint32_t numInvokations, bool child) override final;
        virtual void scheduleSync(const Job &job);
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

        //--

        void dump();
        void manualCounterUnlock(uint32_t id);

    protected:
        static const uint32_t MAX_JOBS = 64 * 1024; // all pending jobs
        static const uint32_t MAX_STACK = 320 * 1024; // max size of the stack for a fiber

        struct PendingJob;
        struct ThreadState;
        struct FiberState;

        typedef void* ThreadHandle;

        struct FiberHandle
        {
            void* fiber = nullptr;
            uint8_t* stackBaser = nullptr;
            uint8_t* stackEndr = nullptr;

            INLINE bool valid() const
            {
                return fiber != nullptr;
            }
        };

        struct FiberState
        {
            BaseScheduler* scheduler = nullptr;
            bool isMainThreadFiber = false;
            FiberHandle fiberHandle;
            FiberState* listNext = nullptr; // internal list
            FiberState* listPrev = nullptr; // internal list

            std::atomic<PendingJob*> currentJob = nullptr;
            std::atomic<bool> isAllocated = false;
            std::atomic<bool> isRunning = false;
        };

        struct ThreadState
        {
            BaseScheduler* scheduler = nullptr;
            FiberState idleFiber;
            ThreadHandle threadHandle;
            uint8_t index = 0;
            char name[50] = {0};
            bool isMainThread = false;

            std::atomic<FiberState*> attachedFiber = nullptr;

            // commands as comming back from the fiber
            std::atomic<FiberState*> fiberToRelease = nullptr;
            std::atomic<PendingJob*> jobToReschedule = nullptr;
        };

        enum class PendingJobState
        {
            Free, // entry is not used
            Scheduled, // job was socket yet run but it's schedule to run
            Running, // job is running
            Yielded, // job has yielded and is waiting to be rerun
            Waiting, // job has entered wait state
            Finished, // job has finished executing
        };

        struct WaitList;

        struct PendingJob
        {
            Job job; // job data
            FiberState* fiber = nullptr; // assigned fiber (only for rescheduled jobs)
            PendingJob* next = nullptr; // next job on the queue or in the wait list
            logging::ILogSink* logSink = nullptr; // log sink attached to the job
            uint64_t sequenceNumber = 0; // original sequence number (chain ID)
            uint32_t invocation = 0; // ID of the invocation
            uint8_t lastThreadIndex = 0xFF; // last thread we were running on
            bool isMainThreadJob = false; // is this a job from the main thread
            bool isAllocated = false; // is this job allocated
            WaitCounter waitForCounter;
            const WaitList* waitList = nullptr; // counter for which we are in the wait list
            JobID jobId = 0; // unique ID of the job
            std::atomic<PendingJobState> state = PendingJobState::Free; // general status of the job

#ifndef BUILD_FINAL
            debug::Callstack waitCallstack; // callstack at the moment of entering wait/yield state
#endif
        };

        class PendingJobPool
        {
        public:
            PendingJobPool();

            PendingJob* alloc();
            void release(PendingJob* job);

        private:
            Mutex lock;

            PendingJob* freeList = nullptr;
            std::atomic<int> numAllocatedJobs = 0;
            std::atomic<int> numFreeJobs = 0;
            std::atomic<uint32_t> nextJobId = 0;
        };

        struct FiberPool
        {
            FiberState* freeFibers = nullptr;
            FiberState* activeFibers = nullptr;
            std::atomic<int> numUsedFibers = 0;
            std::atomic<int> numFreeFibers = 0;
            Mutex m_lock;

            typedef std::function<FiberState*()> TRefillFunction;
            TRefillFunction refill;

            FiberState* allocFiber();
            void releaseFiber(FiberState* state);
            void inspect(const std::function<void(const FiberState* state)>& inspector);
            void validate();
        };

        struct WaitList : public base::NoCopy
        {
            WaitCounterID localId = 0;
            WaitCounterSeqID seqId = 0;
            std::atomic<int> currentCount = 0;
            const char* userName = nullptr;
            PendingJob* jobs = nullptr;
            WaitList* listNext = nullptr;
            WaitList* listPrev = nullptr;
            Event* waitEvent = nullptr;
        };

        struct SyncList : public base::NoCopy
        {
            struct SyncJob
            {
                Job job;
                SyncJob* next = nullptr;
            };

            SyncJob* freeList = nullptr;
            SyncJob* head = nullptr;
            SyncJob* tail = nullptr;
            SpinLock m_lock;
            std::atomic<uint32_t> m_count;

            SyncList();

            void push(const Job& job);
            void run();
        };

        class WaitCounterPool : public base::NoCopy
        {
        public:
            WaitCounterPool();
            ~WaitCounterPool();

            // initialize the wait list
            void init(uint32_t maxWaitLists);

            // create a waiting counter with given initial count
            WaitCounter allocWaitCounter(const char* userName, uint32_t initialCount);

            // get current counter at given ID
            WaitCounter findCounter(uint32_t id);

            // release waiting count
            void releaseWaitCounter(const WaitCounter& counter);

            // check state of the waiting counter
            bool checkWaitCounter(const WaitCounter& counter);

            // register job in the waiting list for given counter
            // returns true if the job should enter waiting state
            bool addJobToWaitingList(const WaitCounter& counter, PendingJob* job);

            // signal give wait counter, returns list of jobs to unlock if the counter was successfully signaled (the count reached zero)
            PendingJob* signalWaitCounter(const WaitCounter& counter, uint32_t count);

            // print counter information
            void printCounterInfo(const WaitCounter& counter);

            // print counter information
            void printCounterInfo(const WaitList& entry);

            // inspect list of active wait counters
            void inspect(const std::function<void(const WaitList* list)>& inspector);

            // validate data structures
            void validate();

            // wait (Event) for a counter to finish
            void waitForCounterThreadEvent(const WaitCounter& counter);

        private:
            Mutex m_lock;

            WaitList* m_waitLists;
            WaitList* m_freeWaitLists;
            WaitList* m_activeWaitLists;
            std::atomic<uint32_t> m_numActiveWaitLists;
            std::atomic<uint32_t> m_seqId;

            uint32_t m_maxCounterId;

            WaitList* alloc_NoLock();
            void release_NoLock(WaitList* entry);
        };

        typedef Array<ThreadState> TThreads;
        TThreads m_threads;

        ThreadState m_mainThreadState;
        FiberState m_mainFiberState;
        PendingJob m_mainJobState;

        FiberPool m_fiberPool;
        PendingJobPool m_pendingJobPool;
        WaitCounterPool m_waitConterPool;
        SyncList m_syncList;

        IOrderedQueue* m_pendingJobsQueue;
        IOrderedQueue* m_mainThreadJobsQueue;

        std::atomic<uint32_t> m_numScheduledJobs;
        std::atomic<uint64_t> m_fiberSequenceNumber;

        //--

        // internal cleanup
        void shutdown();

        // determine number of worker threads to create
        uint32_t determineWorkerThreadCount(const IBaseCommandLine& cmdLine);

        // add job to the proper queue, it will be picked up by free fiber thread once it's not busy
        void schedulePendingJob(PendingJob* job);

        // create internal pending job object
        void scheduleInternal(const Job& job, uint32_t numInvokations, bool child);

        // cleanup finished job
        void cleanupPendingJob(PendingJob* job);

        // get sequence ID of current job
        uint64_t currentJobSequenceId() const;

        //--

        // fiber job processing function
        void fiberFunc(FiberState* state);

        // process fiber job
        void processFiberJob(FiberState* state);

        // process internal fiber schedule
        void schedulerFunc();

        //--

        // get info about active thread
        // NOTE: may return NULL if this thread does not belong to the fiber system
        virtual ThreadState* currentThreadState() const = 0;

        // create fiber processing thread
        virtual ThreadHandle createWorkerThread(void* state) = 0;

        // convert main thread to fiber
        virtual void convertMainThread() = 0;

        // create a fresh fiber state
        virtual FiberHandle createFiberState(uint32_t stackSize, void* state) = 0;

        // switch current thread to a different fiber
        virtual void switchFiber(FiberHandle from, FiberHandle to) = 0;
    };

} // prv

END_BOOMER_NAMESPACE(base::fibers)