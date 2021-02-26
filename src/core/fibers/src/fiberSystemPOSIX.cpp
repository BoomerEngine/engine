/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl\posix #]
* [# platform: posix #]
***/

#include "build.h"
#include "fiberSystemPOSIX.h"

#include "core/system/include/thread.h"
#include "core/system/include/scopeLock.h"

#include <pthread.h>
#include <ucontext.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>

//#define USE_PROTECTED_STACK

namespace base
{
    namespace fibers
    {
        namespace prv
        {

            //--

            pthread_key_t PosixScheduler::GTlsThreadState = 0;

            //--

            PosixScheduler::PosixScheduler()
            {
                pthread_key_create(&GTlsThreadState, NULL);
            }

            PosixScheduler::~PosixScheduler()
            {
                if (m_pendingJobsQueue)
                {
                    m_pendingJobsQueue->close();
                    m_pendingJobsQueue = nullptr;
                }

                flush();

                for (auto &cur : m_threads)
                {
                    pthread_join((pthread_t) cur.m_threadHandle, NULL);
                }
                m_threads.clear();
            }

            namespace helper
            {
                static void* ReassembleAddress(uint32_t lo, uint32_t hi)
                {
                    auto addr = (((uint64_t)hi) << 32) | lo;
                    return (void*)addr;
                }

                static void* AllocateProtectedMemory(uint32_t size)
                {
                    // align the size to the page size
                    auto pageSize = getpagesize();
                    auto alignedSize = Align<uint32_t>(size, pageSize);

                    // get the size of the page
                    auto pageMargins = 4;

                    // allocate the memory needed for the job context
                    auto totalSize = alignedSize + 2*pageMargins*pageSize;
                    auto basePtr  = (uint8_t*)mmap(nullptr, totalSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, 0, 0);
                    if (basePtr == (void*)-1)
                    {
                        FATAL_ERROR(TempString("Failed to allocate protected memory: %u", errno));
                        return nullptr;
                    }

                    // mark the inner memory as read/write
                    auto usablePtr  = basePtr + (pageMargins * pageSize);
                    mprotect(usablePtr, alignedSize, PROT_READ | PROT_WRITE);

                    // fill memory with patern

                    // return the usable pointer
                    return usablePtr;
                }

                static void* AllocateNormalMemory(uint32_t size)
                {
                    return malloc(size);
                }
            }

            PosixScheduler::FiberHandle PosixScheduler::CreateContext(void (*taskFunction)(uint32_t, uint32_t), uint32_t stackSize, const void* paramToPass)
            {
                // allocate space for context and stack
                auto contextPtr  = (ucontext_t*)helper::AllocateNormalMemory(sizeof(ucontext_t));

                // setup stuff
                sigset_t zero;
                sigemptyset(&zero);
                sigprocmask(SIG_BLOCK, &zero, &contextPtr->uc_sigmask);

                // must initialize with current context
                if (getcontext(contextPtr) < 0)
                    return FiberHandle();

                // call makecontext to do the real work
                // leave a few words open on both ends
#ifdef USE_PROTECTED_STACK
                contextPtr->uc_stack.ss_sp = helper::AllocateProtectedMemory(stackSize);
#else
                contextPtr->uc_stack.ss_sp = helper::AllocateNormalMemory(stackSize);
#endif
                contextPtr->uc_stack.ss_size = stackSize - 96;

                // All this magic is because you have to pass makecontext a function that takes some number of word-sized variables, and on 64-bit machines pointers are bigger than words.
                uint64_t valueToPass = (uint64_t)paramToPass;
                uint32_t valueToPassHigh = (uint32_t)(valueToPass >> 32);
                uint32_t valueToPassLow = (uint32_t)(valueToPass);
                makecontext(contextPtr, (void(*)())taskFunction, 2, valueToPassLow, valueToPassHigh);

                // construct fiber handle
                FiberHandle handle;
                handle.m_stackBase = (uint8_t*)contextPtr->uc_stack.ss_sp;
                handle.m_stackEnd = handle.m_stackBase + contextPtr->uc_stack.ss_size;
                handle.m_fiber = contextPtr;
                return handle;
            }

            void* PosixScheduler::ThreadFunc(void* threadParameter)
            {
                // bind thread state to thread
                auto state  = (ThreadState*)threadParameter;
                pthread_setspecific(GTlsThreadState, state);

                // name the thread
                TRACE_SPAM("Entering worker thread '{}'", state->m_name);
                SetThreadName(state->m_name);

                // create the idle fiber for the thread
                state->m_idleFiber.m_currentJob = nullptr;
                state->m_idleFiber.m_scheduler = state->m_scheduler;
                state->m_idleFiber.m_fiberHandle = CreateContext(&SchedulerFiberFunc, 1024, state->m_scheduler);
                state->m_attachedFiber = &state->m_idleFiber;

                // enter the scheduler function
                auto scheduler  = (PosixScheduler*)state->m_scheduler;
                scheduler->schedulerFunc();

                // once we are hear it means we've completed all jobs
                TRACE_SPAM("Exiting worker thread '{}'", state->m_name);
                return 0;
            }

            void PosixScheduler::WorkerFiberFunc(uint32_t fiberParameterLo, uint32_t fiberParameterHi)
            {
                auto state  = (FiberState*)helper::ReassembleAddress(fiberParameterLo, fiberParameterHi);
                auto scheduler  = (PosixScheduler*)state->m_scheduler;
                scheduler->fiberFunc(state);
            }

            void PosixScheduler::SchedulerFiberFunc(uint32_t fiberParameterLo, uint32_t fiberParameterHi)
            {
                auto scheduler  = (PosixScheduler*)helper::ReassembleAddress(fiberParameterLo, fiberParameterHi);
                scheduler->schedulerFunc();
            }

            PosixScheduler::ThreadState* PosixScheduler::currentThreadState() const
            {
                return (ThreadState*)pthread_getspecific(GTlsThreadState);
            }

            PosixScheduler::ThreadHandle PosixScheduler::createWorkerThread(void* state)
            {
                pthread_t handle = 0;

                // Create thread
                pthread_attr_t attrs;
                pthread_attr_init(&attrs);
                pthread_attr_setstacksize(&attrs, 65536);
                pthread_create(&handle, &attrs, &PosixScheduler::ThreadFunc, state);
                pthread_attr_destroy(&attrs);
                if (handle == 0)
                {
                    TRACE_ERROR("Failed to create POSIX thread: {}", errno);
                    return nullptr;
                }

                // return thread handle
                return (ThreadHandle)handle;
            }

            void PosixScheduler::convertMainThread()
            {
                m_mainFiberState.m_fiberHandle = CreateContext(&PosixScheduler::SchedulerFiberFunc, 65536, this);
                m_mainFiberState.m_scheduler = this;
                m_mainFiberState.m_currentJob = nullptr;
                m_mainFiberState.m_isMainThreadFiber = true;
                m_mainFiberState.m_isRunning = true;

                m_mainJobState.m_fiber = &m_mainFiberState;
                m_mainJobState.m_jobId = 0xFFFF;
                m_mainJobState.m_invocation = 0;
                m_mainJobState.m_job.m_func = [](FIBER_FUNC) { ASSERT("!DO NOT CALL"); };
                m_mainJobState.m_job.m_name = "MainThreadJob";
                m_mainJobState.m_job.m_size = Size::Normal;
                m_mainJobState.m_next = nullptr;
                m_mainJobState.m_priority = Priority::High;
                m_mainJobState.m_state = PendingJobState::Running;
                m_mainJobState.m_isMainThreadJob = true;
                m_mainJobState.m_waitList = nullptr;
                m_mainFiberState.m_currentJob = &m_mainJobState;

                m_mainThreadState.m_threadHandle = (ThreadHandle)pthread_self();
                m_mainThreadState.m_threadQueueMask = 1 << (uint8_t)Priority::MainThreadOnly;
                m_mainThreadState.m_scheduler = this;
                m_mainThreadState.m_attachedFiber = &m_mainFiberState;
                m_mainThreadState.m_isMainThread = true;
                m_mainThreadState.m_idleFiber.m_fiberHandle = CreateContext(&PosixScheduler::SchedulerFiberFunc, 65536, this);
                m_mainThreadState.m_idleFiber.m_currentJob = nullptr;
                m_mainThreadState.m_idleFiber.m_scheduler = this;
                strcpy_s(m_mainThreadState.m_name, "MainThread");

                pthread_setspecific(GTlsThreadState, &m_mainThreadState);
            }

            PosixScheduler::FiberHandle PosixScheduler::createFiberState(uint32_t stackSize, void* state)
            {
                return CreateContext(&PosixScheduler::WorkerFiberFunc, stackSize, state);
            }

            void PosixScheduler::switchFiber(FiberHandle from, FiberHandle to)
            {
                swapcontext((ucontext_t*)from.m_fiber, (const ucontext_t*)to.m_fiber);
            }

        } // prv
    } // fibers
} // base
