/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: background #]
***/

#pragma once

#include "core/system/include/commandline.h"
#include "core/system/include/thread.h"
#include "core/containers/include/hashMap.h"
#include "core/system/include/semaphoreCounter.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// background job runner
class CORE_FIBERS_API BackgroundScheduler : public ISingleton
{
    DECLARE_SINGLETON(BackgroundScheduler);

public:
    BackgroundScheduler();

    // initialize background job system
    bool initialize(const IBaseCommandLine& commandline);

    // finish all running and scheduled jobs
    // NOTE: this may be a VERY LONG CALL and should not be called unless something very special happens (like quitting)
    void flush();

    /// run background job
    void schedule(IBackgroundJob* job, StringID bucket);

private:
    struct ThreadBucket
    {
        StringID bucketName;

        SpinLock scheduledJobsLock;
        Array<RefPtr<IBackgroundJob>> scheduledJobs;
        Semaphore scheduledJobsSemaphore;

        RefPtr<IBackgroundJob> currentJob;
        Thread thread;

        std::atomic<bool> canceledFlag = false;

        ThreadBucket();
    };

    HashMap<StringID, ThreadBucket*> m_threadBuckets;
    SpinLock m_threadBucketsLock;

    void runBucket(ThreadBucket* bucket);
};

//--

END_BOOMER_NAMESPACE()
