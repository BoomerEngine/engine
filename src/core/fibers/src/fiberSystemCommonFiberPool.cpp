/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl #]
***/

#include "build.h"
#include "core/system/include/scopeLock.h"
#include "core/system/include/semaphoreCounter.h"
#include "core/system/include/thread.h"
#include "fiberSystemCommon.h"


#ifdef PLATFORM_GCC
    #pragma GCC push_options
    #pragma GCC optimize ("O0")
#else
    #pragma optimize("",off)
#endif

BEGIN_BOOMER_NAMESPACE()

namespace prv
{

    ///---

    BaseScheduler::FiberState* BaseScheduler::FiberPool::allocFiber()
    {
        auto lock = CreateLock(m_lock);

        // pop fiber from the free list
        FiberState* ret = nullptr;
        if (freeFibers)
        {
            ret = freeFibers;
            ASSERT(freeFibers->listPrev == nullptr);
            freeFibers = freeFibers->listNext;
            auto numFree = --numFreeFibers;
            ASSERT(numFree >= 0);
        }
        else
        {
            ret = refill();
        }

        // add fiber to the used list
        {
            ret->listNext = activeFibers;
            ASSERT(ret->listPrev == nullptr);
            if (activeFibers)
            {
                ASSERT(activeFibers->listPrev == nullptr);
                activeFibers->listPrev = ret;
            }
            activeFibers = ret;
            ++numUsedFibers;
        }

        // mark as allocated
        ASSERT(ret->isAllocated == false);
        ret->isAllocated = true;

        return ret;
    }

    void BaseScheduler::FiberPool::releaseFiber(FiberState* state)
    {
        auto lock = CreateLock(m_lock);

        ASSERT_EX(!state->isMainThreadFiber, "Cannot release main fiber");
        ASSERT_EX(state->currentJob.load() == nullptr, "Freeing fiber that is in use");
        ASSERT_EX(state->isAllocated == true, "Trying to release fiber that is allocated");

        // remove fiber from active list
        if (state->listNext)
            state->listNext->listPrev = state->listPrev;
        if (state->listPrev)
            state->listPrev->listNext = state->listNext;
        else
            activeFibers = state->listNext;
        auto numActive = --numUsedFibers;
        ASSERT(numActive >= 0);

        // add to free list
        state->listPrev = nullptr;
        state->listNext = freeFibers;
        freeFibers = state;
        numFreeFibers++;

        // mark as not allocated
        ASSERT(state->isAllocated == true);
        state->isAllocated = false;
    }

    void BaseScheduler::FiberPool::validate()
    {
        auto lock = CreateLock(m_lock);

        {
            uint32_t numFibers = 0;
            for (auto cur  = freeFibers; cur; cur = cur->listNext)
            {
                ASSERT(cur->isAllocated == false);
                ASSERT(cur->listPrev == nullptr);
                ASSERT(cur->isMainThreadFiber == false);
                ASSERT(cur->currentJob.load() == nullptr);
                numFibers += 1;
            }
            ASSERT(numFibers == (uint32_t)numFreeFibers.load());
        }

        {
            uint32_t numFibers = 0;
            for (auto cur  = activeFibers; cur; cur = cur->listNext)
            {
                ASSERT(cur->isAllocated == true);
                ASSERT(cur->currentJob.load() != nullptr);
                numFibers += 1;
            }
            ASSERT(numFibers == (uint32_t)numUsedFibers.load());
        }
    }

    void BaseScheduler::FiberPool::inspect(const std::function<void(const FiberState* state)>& inspector)
    {
        auto lock = CreateLock(m_lock);
        for (auto cur  = activeFibers; cur; cur = cur->listNext)
            inspector(cur);
    }

    ///---

} // prv
    
#ifdef PLATFORM_GCC
    #pragma GCC pop_options
#else
    #pragma optimize("",on)
#endif

END_BOOMER_NAMESPACE()
