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

    BaseScheduler::SyncList::SyncList()
        : freeList(nullptr)
        , head(nullptr)
        , tail(nullptr)
        , m_count(0)
    {}

    void BaseScheduler::SyncList::push(const FiberJob& jobPayload)
    {
        auto lock = CreateLock(m_lock);

        // get job
        SyncJob* job = freeList;
        if (!job)
        {
            job = new SyncJob();
            job->next = nullptr;
        }
        else
        {
            freeList = job->next;
            job->next = nullptr;
        }

        // set payload
        job->job = jobPayload;

        // add to list
        if (head)
        {
            job->next = nullptr;
            tail->next = job;
            tail = job;
        }
        else
        {
            job->next = nullptr;
            head = job;
            tail = job;
        }

        ++m_count;
    }

    void BaseScheduler::SyncList::run()
    {
        auto lock = CreateLock(m_lock);

        if (head != nullptr) // just run once
        {
            auto cur  = head;
            head = nullptr;
            tail = nullptr;
            --m_count;

            // release for the duration of job execution
            lock.release();

            // run jobs
            while (cur)
            {
                cur->job.func(0);
                cur->job.func = TJobFunc();
                cur = cur->next;
            }

            // acquire back
            lock.aquire();

            // release to free list
            while (cur)
            {
                auto next = cur->next;
                cur->next = freeList;
                freeList = cur;
                cur = next;
            }
        }
    }

    ///---

} // prv

END_BOOMER_NAMESPACE()

#ifdef PLATFORM_GCC
    #pragma GCC pop_options
#else
    #pragma optimize("",on)
#endif