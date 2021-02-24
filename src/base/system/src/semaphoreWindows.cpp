/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\winapi #]
* [#platform: windows #]
***/

#include "build.h"
#include "semaphoreCounter.h"

BEGIN_BOOMER_NAMESPACE(base)

Semaphore::Semaphore(uint32_t initialCount, uint32_t maxCount)
{
    m_handle = CreateSemaphore(NULL, initialCount, maxCount, NULL);
}

Semaphore::~Semaphore()
{
    CloseHandle((HANDLE)m_handle);
}

uint32_t Semaphore::release(uint32_t count)
{
    uint32_t previousCount = 0;
    ReleaseSemaphore(m_handle, count, (LPLONG)&previousCount);
    return previousCount;
}

bool Semaphore::wait(const uint32_t waitTime /*INFINITE_TIME*/)
{
    for (;;)
    {
        auto ret = WaitForSingleObjectEx(m_handle, waitTime, TRUE);
        if (ret == WAIT_IO_COMPLETION)
            continue;

        DEBUG_CHECK_EX(ret == WAIT_OBJECT_0 || ret == WAIT_TIMEOUT, "Semaphore lost while waiting for it");
        return (ret == WAIT_OBJECT_0);
    }
}

int Semaphore::WaitMultiple(Semaphore** semaphores, uint32_t numSemaphores, uint32_t waitTime /*= INFINITE_TIME*/)
{
    static const uint32_t MAX_SEMAPHORES = 64;

    // nothing to signal
    if (numSemaphores == 0)
        return 0;

    // prepare list of semaphores
    auto numWaitableSemaphores = std::min<uint32_t>(MAX_SEMAPHORES, numSemaphores);
    HANDLE hSemaphores[MAX_SEMAPHORES];
    for (uint32_t i=0; i<numSemaphores; ++i)
        hSemaphores[i] = (HANDLE)semaphores[i]->m_handle;

    // wait for stuff
    for (;;)
    {
        auto ret = WaitForMultipleObjects(numWaitableSemaphores, hSemaphores, TRUE, waitTime);
        if (ret == WAIT_IO_COMPLETION)
            continue;

        DEBUG_CHECK_EX(ret >= WAIT_OBJECT_0 || ret == WAIT_TIMEOUT, "Semaphore lost while waiting for it");
        if (ret == WAIT_TIMEOUT)
            return INDEX_NONE;

        return (ret - WAIT_OBJECT_0);
    }
}

END_BOOMER_NAMESPACE(base)
