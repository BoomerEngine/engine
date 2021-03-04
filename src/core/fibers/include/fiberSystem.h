/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: public #]
***/

#pragma once

#define FIBER_FUNC uint64_t index

#include "core/system/include/commandline.h"

BEGIN_BOOMER_NAMESPACE()

typedef uint32_t FiberJobID;
typedef uint32_t FiberSemaphoreID;
typedef uint32_t FiberSemaphoreSeqID;

/// a waitable semaphore, jobs can wait for this counter to reach zero
/// counter can be triggered manually or when assigned to a running job batch it's decremented by each finished job
struct FiberSemaphore
{
public:
    INLINE FiberSemaphore()
    {}

    INLINE FiberSemaphore(FiberSemaphoreID id_, FiberSemaphoreSeqID seqId_)
        : localId(id_), seqId(seqId_)
    {}

    INLINE FiberSemaphore(const FiberSemaphore& other) = default;
    INLINE FiberSemaphore(FiberSemaphore&& other) = default;
    INLINE FiberSemaphore& operator=(const FiberSemaphore& other) = default;
    INLINE FiberSemaphore& operator=(FiberSemaphore&& other) = default;

    /// get the wait counter ID
    INLINE FiberSemaphoreID id() const { return localId; }

    /// get the sequential ID
    INLINE FiberSemaphoreSeqID sequenceId() const { return seqId; }

    /// is this empty ?
    INLINE bool empty() const { return seqId == 0; }

private:
    FiberSemaphoreID localId = 0; // ID of the counter (reused)
    FiberSemaphoreSeqID seqId = 0; // Sequential ID of the counter (never reused, allows to detect use of a stale counter)
};

/// job function
typedef std::function<void(FIBER_FUNC)> TJobFunc;

/// job definition (can be static or on stack)
/// jobs are scheduled via the IScheduler
struct FiberJob
{
    TJobFunc func;
    const char* name = nullptr;
    logging::ILogSink* localSink = nullptr;

    INLINE FiberJob()
    {}

    INLINE FiberJob(const TJobFunc& func, const char* name)
        : func(func)
        , name(name)
    {}
};

//--

/// initialize the fiber scheduler
/// this selected best low-level scheduler for given platform
/// NOTE: this may fail for low-level system reasons (not enough physical cores, etc)
extern CORE_FIBERS_API bool InitializeFibers(const IBaseCommandLine& commandline);

/// stop fiber system
extern CORE_FIBERS_API void ShutdownFibers();

// run sync jobs (called from main thread)
extern CORE_FIBERS_API void RunSyncJobs();

// finish all running and scheduled jobs
// NOTE: this may be a VERY LONG CALL and should not be called unless something very special happens (like quitting)
extern CORE_FIBERS_API void FlushFibers();

/// get ID of current job
/// NOTE: this returns zero if we are not inside a job posted to the fiber system (ie. other system thread)
extern CORE_FIBERS_API FiberJobID CurrentJobID();

/// allocate a wait counter with specific value of the counter
/// NOTE: allocating a wait counter with zero count returns empty FiberSemaphore
extern CORE_FIBERS_API FiberSemaphore CreateFence(const char* name, uint32_t count = 1);

/// check if counter has been signaled
/// NOTE: this may return false negative if counter has JUST been signaled but will never return false positive
extern CORE_FIBERS_API bool CheckSemaphore(const FiberSemaphore& counter);

/// schedule a fiber job to run, assume the job structure will NOT be deleted for the duration of the job
/// number of invocations of the job function may be specified
extern CORE_FIBERS_API void ScheduleFiber(const FiberJob& job, bool child = false, uint32_t numInvokations = 1);

/// schedule a sync to run (a job that runs on the main thread in the safe time)
extern CORE_FIBERS_API void ScheduleSync(const FiberJob& job);

/// signal counter, once the counter value gets to zero jobs are unblocked
/// NOTE: signaling counter that was deleted is illegal
extern CORE_FIBERS_API void SignalFence(const FiberSemaphore& counter, uint32_t count = 1);

/// yield current FIBER job
/// the current job that called this function will be put at the end of the queue of it's priority
/// NOTE: it may take a lot of time before the job is revisited
extern CORE_FIBERS_API void YieldFiber();

/// yield current thread without releasing it's active fiber (efectivelly a Sleep)
/// NOTE: this is wasteful
extern CORE_FIBERS_API void YieldThread(uint32_t time=0);

/// block current job until the wait counter reaches zero
/// NOTE: the wait counter must be valid, it's illegal to wait on a counter that has been deleted
/// NOTE: the wait counter will be released when job is unblocked
/// NOTE: it's legal for more than one job to wait for a given counter
extern CORE_FIBERS_API void WaitForFence(const FiberSemaphore& counter);

/// block current job until all counters finish
extern CORE_FIBERS_API void WaitForMultipleFences(const FiberSemaphore* counters, uint32_t count);

/// true if we are on the so called "MainThread"
/// the main thread has more privileges than other threads, mainly all the app windows are created and managed by it
extern CORE_FIBERS_API bool IsMainThread();

/// true if we are in the main application context (the original fiber that was there when application started)
/// this context has the biggest stack
extern CORE_FIBERS_API bool IsMainFiber();

/// get number of fiber worker threads (without main thread)
extern CORE_FIBERS_API uint32_t WorkerThreadCount();

//--

// helper class for building the instance of runnable job
class CORE_FIBERS_API FiberBuilder
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

//--

extern CORE_FIBERS_API FiberBuilder RunFiber(const char* name = "InlineFiber");

// run a child fiber
extern CORE_FIBERS_API FiberBuilder RunChildFiber(const char* name = "ChildFiber");

// run code at the "safe time" on the main thread, triggered by the app itself in the main loop
// this is excellent to deffer final "update" of stuff to the main thread instead of making everything threadsafe
extern CORE_FIBERS_API FiberBuilder RunSync(const char* name = "Sync");

//--

// run given function N times
extern CORE_FIBERS_API void RunFiberLoop(const char* name, uint32_t count, int maxConcurency, const std::function<void(uint32_t index)>& func);

// run given function N times
template< typename T >
INLINE void RunFiberForFeach(const char* name, const Array<T>& data, int maxConcurency, const std::function<void(const T & data)>& func)
{
    RunFiberLoop(name, data.size(), maxConcurency, [&data, &func](uint32_t index)
        {
            func(data[index]);
        });
}

// run given function N times
template< typename T >
INLINE void RunFiberForFeach(const char* name, Array<T>& data, int maxConcurency, const std::function<void(T & data)>& func)
{
    RunFiberLoop(name, data.size(), maxConcurency, [&data, &func](uint32_t index)
        {
            func(data[index]);
        });
}

//--

END_BOOMER_NAMESPACE()

//--

/// print list of active fibers (debugging)
extern CORE_FIBERS_API void DumpFibers();

/// manually unlock hung counter (debugging)
extern CORE_FIBERS_API void UnlockCounterManual(uint32_t id);

//--
