/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl #]
***/

#include "build.h"
#include "base/system/include/scopeLock.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/system/include/thread.h"
#include "fiberSystemCommon.h"


#ifdef PLATFORM_GCC
    #pragma GCC push_options
    #pragma GCC optimize ("O0")
#else
    #pragma optimize("",off)
#endif

BEGIN_BOOMER_NAMESPACE(base::fibers)

namespace prv
{

    ///---

    BaseScheduler::PendingJobPool::PendingJobPool()
        : freeList(nullptr)
        , numAllocatedJobs(0)
        , numFreeJobs(0)
    {}

    BaseScheduler::PendingJob* BaseScheduler::PendingJobPool::alloc()
    {
        auto lock = CreateLock(this->lock);

        auto job  = freeList;
        if (job == nullptr)
        {
            job = new PendingJob();
        }
        else
        {
            freeList = job->next;
            job->next = nullptr;

            auto count = --numFreeJobs;
            ASSERT(count >= 0);
        }

        ++numAllocatedJobs;

        ASSERT(!job->isAllocated);
        ASSERT(!job->jobId);
        ASSERT(!job->next);
        job->isAllocated = true;
        job->jobId = ++nextJobId;

        return job;
    }

    void BaseScheduler::PendingJobPool::release(PendingJob* job)
    {
        auto lock = CreateLock(this->lock);

        ASSERT(!job->isMainThreadJob);
        ASSERT(job->isAllocated);
        ASSERT(job->next == nullptr);
        ASSERT(job->waitList == nullptr);
        ASSERT(job->jobId != 0);

        job->isAllocated = false;
        job->job.name = nullptr;
        job->job.func = TJobFunc();
        job->fiber = nullptr;
        job->jobId = 0;

        job->next = freeList;
        freeList = job;

        auto count = ++numAllocatedJobs;
        ASSERT(count >= 0);
        ++numFreeJobs;
    }

    ///---

} // prv

END_BOOMER_NAMESPACE(base::fibers)

#ifdef PLATFORM_GCC
    #pragma GCC pop_options
#else
    #pragma optimize("",on)
#endif