/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: public #]
***/

#include "build.h"
#include "fiberSystem.h"
#include "fiberSystemImpl.h"
#include "fiberSystemThreadBased.h"
#include "backgroundJobImpl.h"

#if defined(PLATFORM_WINDOWS)
    #include "fiberSystemWinApi.h"
    typedef boomer::prv::WinApiScheduler FiberSchedulerClass;
#elif defined(PLATFORM_POSIX)
    #include "fiberSystemPOSIX.h"
    typedef boomer::prv::PosixScheduler FiberSchedulerClass;
#else
    typedef boomer::prv::ThreadBasedScheduler FiberSchedulerClass;
#endif

BEGIN_BOOMER_NAMESPACE()

extern prv::IFiberScheduler* GFibers = nullptr;

bool InitializeFibers(const IBaseCommandLine& commandline)
{
    DEBUG_CHECK_RETURN_EX_V(!GFibers, "Fibers already initialized", false);

    // initialize the background job system
    if (!BackgroundScheduler::GetInstance().initialize(commandline))
    {
        TRACE_ERROR("Low-level background job syste failed to initialize");
        return false;
    }

    // use the thread based one if needed
    auto fiberMode = commandline.singleValueAnsiStr("fiberMode");

    // use the platform specified scheduler
    if (0 == strcmp(fiberMode, "threads"))
        GFibers = new prv::ThreadBasedScheduler;
    else
        GFibers = new FiberSchedulerClass;

    // initialize the scheduler
    if (!GFibers->initialize(commandline))
    {
        TRACE_ERROR("Low-level scheduler failed to initialize");
        delete GFibers;
        GFibers = nullptr;
        return false;
    }

    // low-level initialization was ok
    return true;
}

void ShutdownFibers()
{
    // stop all background jobs
    BackgroundScheduler::GetInstance().flush();

    // stop all fibers
    if (GFibers)
    {
        delete GFibers;
        GFibers = nullptr;
    }
}

void FlushFibers()
{
    DEBUG_CHECK_RETURN_EX(GFibers, "Fibers not initialized");
    GFibers->flush();
}

FiberJobID CurrentJobID()
{
    DEBUG_CHECK_RETURN_EX_V(GFibers, "Fibers not initialized", 0);
    return GFibers->currentJobID();
}

FiberSemaphore CreateFence(const char* name, uint32_t count /*= 1*/)
{
    DEBUG_CHECK_RETURN_EX_V(GFibers, "Fibers not initialized", FiberSemaphore());
    DEBUG_CHECK_RETURN_EX_V(count, "Invalid count", FiberSemaphore());
    return GFibers->createCounter(name, count);
}

bool CheckSemaphore(const FiberSemaphore& counter)
{
    if (counter.empty())
        return true;

    DEBUG_CHECK_RETURN_EX_V(GFibers, "Fibers not initialized", false);
    return GFibers->checkCounter(counter);
}

void ScheduleFiber(const FiberJob& job, bool child, uint32_t numInvokations /*= 1*/)
{
    DEBUG_CHECK_RETURN_EX(GFibers, "Fibers not initialized");
    DEBUG_CHECK_RETURN_EX(job.func, "Invalid job definition");
    DEBUG_CHECK_RETURN_EX(numInvokations, "Invalid invocation count");
    GFibers->scheduleFiber(job, numInvokations, child);
}

void ScheduleSync(const FiberJob& job)
{
    DEBUG_CHECK_RETURN_EX(GFibers, "Fibers not initialized");
    DEBUG_CHECK_RETURN_EX(job.func, "Invalid job definition");
    GFibers->scheduleSync(job);
}

void RunSyncJobs()
{
    DEBUG_CHECK_RETURN_EX(GFibers, "Fibers not initialized");
    GFibers->runSyncJobs();
}

void SignalFence(const FiberSemaphore& counter, uint32_t count /*= 1*/)
{
    DEBUG_CHECK_RETURN_EX(GFibers, "Fibers not initialized");
    DEBUG_CHECK_RETURN_EX(!counter.empty(), "Invalid semaphore to signal");
    DEBUG_CHECK_RETURN_EX(count, "Invalid event count");
    return GFibers->signalCounter(counter);
}

void YieldFiber()
{
    DEBUG_CHECK_RETURN_EX(GFibers, "Fibers not initialized");
    GFibers->yieldCurrentJob();
}

void YieldThread()
{

}

void WaitForFence(const FiberSemaphore& counter)
{
    DEBUG_CHECK_RETURN_EX(GFibers, "Fibers not initialized");

    if (!counter.empty())
        GFibers->waitForCounterAndRelease(counter);
}

void WaitForMultipleFences(const FiberSemaphore* counters, uint32_t count)
{
    DEBUG_CHECK_RETURN_EX(GFibers, "Fibers not initialized");
    GFibers->waitForMultipleCountersAndRelease(counters, count);
}

bool IsMainThread()
{
    return GFibers ? GFibers->isMainThread() : true;
}

bool IsMainFiber()
{
    return GFibers ? GFibers->isMainFiber() : true;
}

uint32_t WorkerThreadCount()
{
    return GFibers ? GFibers->workerThreadCount() : 1;
}

//---

FiberBuilder::FiberBuilder(const char* name /*= "InlineFiber"*/, uint32_t numInvokations /*= 1*/, bool child /* = false */, bool sync /*= false*/)
    : m_name(name)
    , m_sync(sync)
    , m_child(child)
    , m_numInvocations(numInvokations)
{}

FiberBuilder::FiberBuilder(const FiberBuilder& other)
    : m_name(other.m_name)
    , m_numInvocations(other.m_numInvocations)
    , m_child(other.m_child)
    , m_sync(other.m_sync)
{}

void FiberBuilder::operator<<(const TJobFunc& job)
{
    m_func = job;
}

FiberBuilder::~FiberBuilder()
{
    if (m_func)
    {
        if (m_sync)
        {
            FiberJob job;
            job.func = std::move(m_func);
            job.name = m_name;
            job.localSink = m_propagateLogSink ? logging::Log::GetCurrentLocalSink() : nullptr;

            GFibers->scheduleSync(job);
        }
        else
        {
            FiberJob job;
            job.func = std::move(m_func);
            job.name = m_name;
            job.localSink = m_propagateLogSink ? logging::Log::GetCurrentLocalSink() : nullptr;

            GFibers->scheduleFiber(job, m_child, m_numInvocations);
        }
    }
}

//--

FiberBuilder RunFiber(const char* name /*= "InlineFiber"*/)
{
    return FiberBuilder(name, 1, false, false);
}

FiberBuilder RunChildFiber(const char* name /*="ChildFiber"*/)
{
    return FiberBuilder(name, 1, true, false);
}

FiberBuilder RunSync(const char* name /*= "InlineFiber"*/)
{
    return FiberBuilder(name, 1, false, true);
}

//----

void RunFiberLoop(const char* name, uint32_t count, int maxConcurency, const std::function<void(uint32_t index)>& func)
{
    if (count > 0)
    {
        if (count == 1)
        {
            func(0);
        }
        else
        {
            auto jobDone = GFibers->createCounter(name, count);

            RunChildFiber(name).invocations(count) << [jobDone, &func](FIBER_FUNC)
            {
                func(index);
                GFibers->signalCounter(jobDone);
            };

            GFibers->waitForCounterAndRelease(jobDone);
        }
    }
}

//--

END_BOOMER_NAMESPACE()