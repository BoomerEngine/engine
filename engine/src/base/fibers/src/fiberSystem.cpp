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

#if defined(PLATFORM_WINDOWS)
    #include "fiberSystemWinApi.h"
    typedef base::fibers::prv::WinApiScheduler FiberSchedulerClass;
#elif defined(PLATFORM_POSIX)
    #include "fiberSystemPOSIX.h"
    typedef base::fibers::prv::PosixScheduler FiberSchedulerClass;
#else
    typedef base::fibers::prv::ThreadBasedScheduler FiberSchedulerClass;
#endif

namespace base
{
    namespace fibers
    {

        Scheduler::Scheduler()
            : m_scheduler(nullptr)
        {
        }

        bool Scheduler::initialize(const IBaseCommandLine& commandline)
        {
            ASSERT_EX(m_scheduler == nullptr, "Fiber scheduler already initialized");

            // use the thread based one if needed
            auto fiberMode = commandline.singleValueAnsiStr("fiberMode");

            // use the platform specified scheduler
            if (0 == strcmp(fiberMode,"threads"))
                m_scheduler = MemNew(prv::ThreadBasedScheduler);
            else
                m_scheduler = MemNew(FiberSchedulerClass);

            // initialize the scheduler
            if (!m_scheduler->initialize(commandline))
            {
                TRACE_ERROR("Low-level scheduler failed to initialize");
                MemDelete(m_scheduler);
                m_scheduler = nullptr;
                return false;
            }

            // TODO: integrate with PC

            // low-level initialiation was ok
            return true;
        }

        void Scheduler::deinit()
        {
            if (m_scheduler)
            {
                MemDelete(m_scheduler);
                m_scheduler = nullptr;
            }
        }

        void Scheduler::flush()
        {
            PC_SCOPE_LVL1(SchedulerFlush);
            if (m_scheduler)
                m_scheduler->flush();
        }

        JobID Scheduler::currentJobID()
        {
            return m_scheduler ? m_scheduler->currentJobID() : 0;
        }

        WaitCounter Scheduler::createCounter(const char* name, uint32_t count /*= 1*/)
        {
            if (count == 0)
                return WaitCounter();

            ASSERT(m_scheduler != nullptr);
            return m_scheduler->createCounter(name, count);
        }

        bool Scheduler::checkCounter(const WaitCounter& counter)
        {
            if (counter.empty())
                return true;

            ASSERT(m_scheduler != nullptr);
            return m_scheduler->checkCounter(counter);
        }

        void Scheduler::scheduleFiber(const Job& job, bool child, uint32_t numInvokations /*= 1*/)
        {
            if (numInvokations > 0)
            {
                ASSERT(m_scheduler != nullptr);
                m_scheduler->scheduleFiber(job, numInvokations, child);
            }
        }

        void Scheduler::scheduleSync(const Job& job)
        {
            m_scheduler->scheduleSync(job);
        }

        void Scheduler::runSyncJobs()
        {
            m_scheduler->runSyncJobs();
        }

        void Scheduler::signalCounter(const WaitCounter& counter, uint32_t count /*= 1*/)
        {
            if (!counter.empty())
            {
                ASSERT(m_scheduler != nullptr);
                return m_scheduler->signalCounter(counter);
            }
        }

        void Scheduler::yield()
        {
            if (m_scheduler)
                m_scheduler->yieldCurrentJob();
        }

        void Scheduler::waitForCounterAndRelease(const WaitCounter& counter)
        {
            ASSERT(m_scheduler != nullptr);
            m_scheduler->waitForCounterAndRelease(counter);
        }

        void Scheduler::waitForMultipleCountersAndRelease(const WaitCounter* counters, uint32_t count)
        {
            ASSERT(m_scheduler != nullptr);
            m_scheduler->waitForMultipleCountersAndRelease(counters, count);
        }

        bool Scheduler::isMainThread()
        {
            return m_scheduler ? m_scheduler->isMainThread() : true;
        }

        bool Scheduler::isMainFiber()
        {
            return m_scheduler ? m_scheduler->isMainFiber() : true;
        }

        uint32_t Scheduler::workerThreadCount()
        {
            return m_scheduler ? m_scheduler->workerThreadCount() : 1;
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
                    Job job;
                    job.func = std::move(m_func);
                    job.name = m_name;
                    job.localSink = m_propagateLogSink ? logging::Log::GetCurrentLocalSink() : nullptr;

                    Scheduler::GetInstance().scheduleSync(job);
                }
                else
                {
                    Job job;
                    job.func = std::move(m_func);
                    job.name = m_name;
                    job.localSink = m_propagateLogSink ? logging::Log::GetCurrentLocalSink() : nullptr;

                    Scheduler::GetInstance().scheduleFiber(job, m_child, m_numInvocations);
                }
            }
        }

        //---

    } // fibers
} // base

//----

base::fibers::FiberBuilder RunFiber(const char* name /*= "InlineFiber"*/)
{
    return base::fibers::FiberBuilder(name, 1, false, false);
}

base::fibers::FiberBuilder RunChildFiber(const char* name /*="ChildFiber"*/)
{
    return base::fibers::FiberBuilder(name, 1, true, false);
}

base::fibers::FiberBuilder RunSync(const char* name /*= "InlineFiber"*/)
{
    return base::fibers::FiberBuilder(name, 1, false, true);
}

//----

void RunFiberLoop(const char* name, uint32_t count, int maxConcurency, const std::function<void(uint32_t index)>& func)
{
    if (count > 0)
    {
        auto jobDone = Fibers::GetInstance().createCounter(name, count);

        RunChildFiber(name).invocations(count) << [jobDone, &func](FIBER_FUNC)
        {
            func(index);
            Fibers::GetInstance().signalCounter(jobDone);
        };

        Fibers::GetInstance().waitForCounterAndRelease(jobDone);
    }
}

//--