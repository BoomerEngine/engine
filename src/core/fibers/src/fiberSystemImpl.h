/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: impl #]
***/

#pragma once

#include "core/system/include/commandline.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{

    /// implementation interface of fiber scheduler (job system)
    class IFiberScheduler : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_FIBERS)

    public:
        IFiberScheduler();
        virtual ~IFiberScheduler();

        // initialize based on commandline and internal config
        // NOTE: this can fail under some circumstances (like trying to run on a dual-core machine)
        virtual bool initialize(const IBaseCommandLine &commandline) = 0;

        // finish all running and scheduled jobs
        // NOTE: this may be a VERY LONG CALL and should not be called unless something very special happens (like quitting)
        virtual void flush() = 0;

        // run sync jobs that must run on the main thread in specific time window
        virtual void runSyncJobs() = 0;

        // schedule fiber job
        virtual void scheduleFiber(const FiberJob &job, uint32_t numInvokations, bool child) = 0;

        // schedule sync job
        virtual void scheduleSync(const FiberJob &job) = 0;

        // create synchronization counter with given initial count
        // NOTE: creating synchronization counter with count=0 is illegal
        virtual FiberSemaphore createCounter(const char* userName, uint32_t count = 1) = 0;

        // check state of the counter
        virtual bool checkCounter(const FiberSemaphore& counter) = 0;

        // signal previously created synchronization counter, will unblock any waiting jobs
        virtual void signalCounter(const FiberSemaphore &counter, uint32_t count = 1) = 0;

        // wait for counter to reach zero
        virtual void waitForCounterAndRelease(const FiberSemaphore &counter) = 0;

        // wait for multiple counters
        virtual void waitForMultipleCountersAndRelease(const FiberSemaphore* counters, uint32_t count) = 0;

        // yield current job (put's it at the back of the queue)
        virtual void yieldCurrentJob() = 0;

        // are we on the main thread (the thread that started the application) ?
        // NOTE: we may be on the main thread but not in the main fiber
        virtual bool isMainThread() const = 0;

        // are we in the original thread/fiber that started application ?
        virtual bool isMainFiber() const = 0;

        // get number of worker threads
        virtual uint32_t workerThreadCount() const = 0;

        // get numerical ID of the current job being executed, 0 for main fiber
        virtual FiberJobID currentJobID() const = 0;
    };

} // prv

END_BOOMER_NAMESPACE()
