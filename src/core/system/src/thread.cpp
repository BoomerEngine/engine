/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#include "build.h"
#include "thread.h"

#ifdef PLATFORM_WINDOWS
    #include "threadWindows.h"
#elif defined(PLATFORM_POSIX)
    #include "threadPOSIX.h"
#endif

BEGIN_BOOMER_NAMESPACE()

//---

ThreadSetup::ThreadSetup()
    : m_name("BoomerThread")
    , m_priority(ThreadPriority::Normal)
    , m_stackSize(64 * 1024)
{}

//---

Thread::Thread()
    : m_systemThreadHandle(0)
{}

Thread::Thread(Thread&& other)
    : m_systemThreadHandle(other.m_systemThreadHandle)
{
    other.m_systemThreadHandle = 0;
}

Thread& Thread::operator=(Thread&& other)
{
    if (this != &other)
    {
        close();
        m_systemThreadHandle = other.m_systemThreadHandle;
        other.m_systemThreadHandle = 0;
    }

    return *this;
}

Thread::~Thread()
{
    close();
}

void Thread::init(const ThreadSetup& setup)
{
#if defined(PLATFORM_WINDOWS)
    prv::WinThread::Init(&m_systemThreadHandle, setup);
#elif defined(PLATFORM_POSIX)
    prv::POSIXThread::Init(&m_systemThreadHandle, setup);
#else
    #error "Implement this"
#endif
}

void Thread::close()
{
#if defined(PLATFORM_WINDOWS)
    prv::WinThread::Close(&m_systemThreadHandle);
#elif defined(PLATFORM_POSIX)
    prv::POSIXThread::Close(&m_systemThreadHandle);
#else
    #error "Implement this"
#endif
}

//---

//! Lock section
void SpinLock::acquire()
{
    uint32_t spinCount = 0;
#if defined(BUILD_FINAL) || defined(BUILD_RELEASE)
    auto threadID = 1; // don't use thread ID on final builds it's to costly
#else
    auto threadID = GetThreadID();
#endif
    uint32_t value = 0;
    while (!owner.compare_exchange_strong(value, threadID))
    {
        value = 0;

        spinCount += 1;
        if (spinCount == 1024)
        {
            InternalYield();
            spinCount = 0;
        }
    }
}

//! Releases the lock on the critical section
void SpinLock::release()
{
#if defined(BUILD_DEBUG)
    auto curThread = GetThreadID();
#endif
    auto prevThread = owner.exchange(0);
#if defined(BUILD_DEBUG)
    ASSERT_EX(prevThread == curThread, "Only the thread that acquired the SpinLock can release it! Did you Wait for a fiber inside a spinlock ?");
#endif
}

void SpinLock::InternalYield()
{
    Yield();
}

uint32_t SpinLock::GetThreadID()
{
    return GetCurrentThreadID();
}

END_BOOMER_NAMESPACE()
