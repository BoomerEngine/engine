/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: public #]
***/

#pragma once

#define FIBER_FUNC uint64_t index

#include "base/system/include/commandline.h"

namespace base
{
    namespace fibers
    {

        namespace prv
        {
            class IFiberScheduler;
        } // prv

        typedef uint32_t JobID;
        typedef uint32_t WaitCounterID;
        typedef uint32_t WaitCounterSeqID;

		class WaitList;

        /// a waitable counter, jobs can wait for this counter to reach zero
        /// counter can be triggered manually or when assigned to a running job batch it's decremented by each finished job
        struct WaitCounter
        {
        public:
            INLINE WaitCounter()
            {}

            INLINE WaitCounter(WaitCounterID id_, WaitCounterSeqID seqId_)
                : localId(id_), seqId(seqId_)
            {}

            INLINE WaitCounter(const WaitCounter& other) = default;
            INLINE WaitCounter(WaitCounter&& other) = default;
            INLINE WaitCounter& operator=(const WaitCounter& other) = default;
            INLINE WaitCounter& operator=(WaitCounter&& other) = default;

            /// get the wait counter ID
            INLINE WaitCounterID id() const { return localId; }

            /// get the sequential ID
            INLINE WaitCounterSeqID sequenceId() const { return seqId; }

            /// is this empty ?
            INLINE bool empty() const { return seqId == 0; }

        private:
            WaitCounterID localId = 0; // ID of the counter (reused)
            WaitCounterSeqID seqId = 0; // Sequential ID of the counter (never reused, allows to detect use of a stale counter)
        };

        /// job function
        typedef std::function<void(FIBER_FUNC)> TJobFunc;

        /// job definition (can be static or on stack)
        /// jobs are scheduled via the IScheduler
        struct Job
        {
            TJobFunc func;
            const char* name = nullptr;
            logging::ILogSink* localSink = nullptr;

            INLINE Job()
            {}

            INLINE Job(const TJobFunc& func, const char* name)
                : func(func)
                , name(name)
            {}
        };

        /// fiber scheduler (job system)
        class BASE_FIBERS_API Scheduler : public ISingleton
        {
            DECLARE_SINGLETON(Scheduler);

        public:
            Scheduler();

            ///---

            /// initialize the fiber scheduler
            /// this selected best low-level scheduler for given platform
            /// NOTE: this may fail for low-level system reasons (not enough physical cores, etc)
            bool initialize(const IBaseCommandLine& commandline);

            // run sync jobs (called from main thread)
            void runSyncJobs();

            // finish all running and scheduled jobs
            // NOTE: this may be a VERY LONG CALL and should not be called unless something very special happens (like quitting)
            void flush();

            /// get ID of current job
            /// NOTE: this returns zero if we are not inside a job posted to the fiber system (ie. other system thread)
            JobID currentJobID();

            /// allocate a wait counter with specific value of the counter
            /// NOTE: allocating a wait counter with zero count returns empty WaitCounter
            WaitCounter createCounter(const char* name, uint32_t count = 1);

            /// check if counter has been signaled
            /// NOTE: this may return false negative if counter has JUST been signaled but will never return false positive
            bool checkCounter(const WaitCounter& counter);

            /// schedule a fiber job to run, assume the job structure will NOT be deleted for the duration of the job
            /// number of invocations of the job function may be specified
            void scheduleFiber(const Job& job, bool child = false, uint32_t numInvokations = 1);

            /// schedule a sync to run (a job that runs on the main thread in the safe time)
            void scheduleSync(const Job& job);

            /// signal counter, once the counter value gets to zero jobs are unblocked
            /// NOTE: signaling counter that was deleted is illegal
            void signalCounter(const WaitCounter& counter, uint32_t count = 1);

            /// yield current job
            /// the current job that called this function will be put at the end of the queue of it's priority
            /// NOTE: it may take a lot of time before the job is revisited
            void yield();

            /// block current job until the wait counter reaches zero
            /// NOTE: the wait counter must be valid, it's illegal to wait on a counter that has been deleted
            /// NOTE: the wait counter will be released when job is unblocked
            /// NOTE: it's legal for more than one job to wait for a given counter
            void waitForCounterAndRelease(WaitCounter& counter);

            /// block current job until all counters finish
            void waitForMultipleCountersAndRelease(const WaitCounter* counters, uint32_t count);

            /// true if we are on the so called "MainThread"
            /// the main thread has more privileges than other threads, mainly all the app windows are created and managed by it
            bool isMainThread();

            /// true if we are in the main application context (the original fiber that was there when application started)
            /// this context has the biggest stack
            bool isMainFiber();

            /// get number of fiber worker threads (without main thread)
            uint32_t workerThreadCount();

        protected:
            // ISigleton
            virtual void deinit() override;

            // actual low-level fiber scheduler
            prv::IFiberScheduler* m_scheduler;
        };

        //--

        // helper class for building the instance of runnable job
        class BASE_FIBERS_API FiberBuilder
        {
        public:
            FiberBuilder(const char* name = "InlineFiber", uint32_t numInvokations = 1, bool child = false, bool sync = false);
            FiberBuilder(const FiberBuilder& other);
            ~FiberBuilder(); // runs the fiber

            // do not schedule
            INLINE FiberBuilder& cancel()
            {
                m_func = TJobFunc();
                return *this;
            }

            // set fiber name
            // NOTE: string must be immutable and static
            INLINE FiberBuilder& name(const char* name)
            {
                m_name = name;
                return *this;
            }

            // set invocation count
            // NOTE: string must be immutable and static
            INLINE FiberBuilder& invocations(uint32_t numInvocations = 1)
            {
                m_numInvocations = numInvocations;
                return *this;
            }

            // set the normal priority for the fiber
            INLINE FiberBuilder& child(bool flag=true)
            {
                m_child = flag;
                return *this;
            }

            // allow the local log sink propagation (at your own risk - the local log sink MUST be valid till all the fibers finish)
            INLINE FiberBuilder& propagateLogSink()
            {
                m_propagateLogSink = true;
                return *this;
            }

            // assign the function
            void operator<<(const TJobFunc& job);

        private:
            TJobFunc m_func;

            const char* m_name = "InlineFiber";
            uint32_t m_numInvocations = 1;
            bool m_propagateLogSink = false;
            bool m_child = false;
            bool m_sync = false;
        };

    } // fibers
} // base

// the global interface
using Fibers = base::fibers::Scheduler;

// the "GO" style runner
extern BASE_FIBERS_API base::fibers::FiberBuilder RunFiber(const char* name = "InlineFiber");

// run a child fiber
extern BASE_FIBERS_API base::fibers::FiberBuilder RunChildFiber(const char* name = "ChildFiber");

// run code at the "safe time" on the main thread, triggered by the app itself in the main loop
// this is excellent to deffer final "update" of stuff to the main thread instead of making everything threadsafe
extern BASE_FIBERS_API base::fibers::FiberBuilder RunSync(const char* name = "Sync");

//--

// run given function N times
extern BASE_FIBERS_API void RunFiberLoop(const char* name, uint32_t count, int maxConcurency, const std::function<void(uint32_t index)>& func);

// run given function N times
template< typename T >
INLINE void RunFiberForFeach(const char* name, const base::Array<T>& data, int maxConcurency, const std::function<void(const T& data)>& func)
{
    RunFiberLoop(name, data.size(), maxConcurency, [&data, &func](uint32_t index)
        {
            func(data[index]);
        });
}

// run given function N times
template< typename T >
INLINE void RunFiberForFeach(const char* name, base::Array<T>& data, int maxConcurency, const std::function<void(T & data)>& func)
{
    RunFiberLoop(name, data.size(), maxConcurency, [&data, &func](uint32_t index)
        {
            func(data[index]);
        });
}
