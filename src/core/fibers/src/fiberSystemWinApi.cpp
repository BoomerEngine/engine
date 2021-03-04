/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl\winapi #]
* [# platform: winapi #]
***/

#include "build.h"

#include "core/containers/include/stringBuf.h"
#include "core/containers/include/stringID.h"
#include "core/system/include/thread.h"
#include "core/containers/include/stringBuilder.h"
#include "core/system/include/scopeLock.h"

#include "fiberSystemWinApi.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{
    //--

    const uint32_t WinApiScheduler::GTlsThreadState = TlsAlloc();

    WinApiScheduler::WinApiScheduler()
    {}

    WinApiScheduler::~WinApiScheduler()
    {
        m_pendingJobsQueue->close();

//                flush();

        for (auto &cur : m_threads)
        {
            WaitForSingleObject(cur.threadHandle, 1000);
            CloseHandle(cur.threadHandle);
        }
        m_threads.clear();
    }

    __declspec(noinline) WinApiScheduler::ThreadState *WinApiScheduler::currentThreadState() const
    {
        return (ThreadState *) TlsGetValue(GTlsThreadState);
    }

    WinApiScheduler::ThreadHandle WinApiScheduler::createWorkerThread(void *state)
    {
        auto workerThreadStack = 65536;
        return (ThreadHandle) CreateThread(NULL, workerThreadStack, &ThreadFunc, state, 0, NULL);
    }

    void WinApiScheduler::convertMainThread()
    {
        ConvertThreadToFiber(NULL);

        m_mainFiberState.fiberHandle.fiber = GetCurrentFiber();
        m_mainFiberState.scheduler = this;
        m_mainFiberState.currentJob = nullptr;
		m_mainFiberState.isMainThreadFiber = true;
		m_mainFiberState.isRunning = true;

		m_mainJobState.fiber = &m_mainFiberState;
		m_mainJobState.jobId = 0xFFFF;
		m_mainJobState.invocation = 0;
		m_mainJobState.job.func = [](FIBER_FUNC) { ASSERT("!DO NOT CALL"); };
		m_mainJobState.job.name = "MainThreadJob";
		m_mainJobState.next = nullptr;
		m_mainJobState.sequenceNumber = 0;
		m_mainJobState.state = PendingJobState::Running;
		m_mainJobState.isMainThreadJob = true;
		m_mainJobState.waitList = nullptr;
		m_mainFiberState.currentJob = &m_mainJobState;

        m_mainThreadState.threadHandle = GetCurrentThread();
		m_mainThreadState.scheduler = this;
		m_mainThreadState.attachedFiber = &m_mainFiberState;
		m_mainThreadState.isMainThread = true;
		m_mainThreadState.idleFiber.fiberHandle.fiber = CreateFiber(1024, &SchedulerFiberFunc, this);
        m_mainThreadState.idleFiber.currentJob = nullptr;
        m_mainThreadState.idleFiber.scheduler = this;
		m_mainThreadState.attachedFiber = &m_mainFiberState;
        strcpy_s(m_mainThreadState.name, "MainThread");

        TlsSetValue(GTlsThreadState, &m_mainThreadState);
    }

    WinApiScheduler::FiberHandle WinApiScheduler::createFiberState(uint32_t stackSize, void *state)
    {
        WinApiScheduler::FiberHandle ret;
        ret.fiber = CreateFiber(stackSize, &WorkerFiberFunc, state);
        return ret;
    }

    void WinApiScheduler::switchFiber(FiberHandle from, FiberHandle to)
    {
        SwitchToFiber(to.fiber);
    }

    //---

    DWORD WinApiScheduler::ThreadFunc(void* threadParameter)
    {
        // bind thread state to thread
        auto state  = (ThreadState*)threadParameter;
        TlsSetValue(GTlsThreadState, state);

        // name the thread
        TRACE_SPAM("Entering worker thread '{}'", state->name);
        SetThreadName(state->name);

        // create the idle fiber for the thread
        ConvertThreadToFiber(nullptr);
        state->idleFiber.currentJob = nullptr;
        state->idleFiber.fiberHandle.fiber = GetCurrentFiber();
        state->idleFiber.scheduler = state->scheduler;
        state->attachedFiber = &state->idleFiber;

        // enter the idle fiber
        auto scheduler  = (WinApiScheduler*)state->scheduler;
        scheduler->schedulerFunc();

        // once we are hear it means we've completed all jobs
        TRACE_SPAM("Exiting worker thread '{}'", state->name);
        return 0;
    }

    void WinApiScheduler::WorkerFiberFunc(void* fiberParameter)
    {
        auto state  = (FiberState*)fiberParameter;
        auto scheduler  = (WinApiScheduler*)state->scheduler;
        scheduler->fiberFunc(state);
    }

    void WinApiScheduler::SchedulerFiberFunc(void* fiberParameter)
    {
        auto scheduler  = (WinApiScheduler*)fiberParameter;
        scheduler->schedulerFunc();
    }

} // prv

END_BOOMER_NAMESPACE()
