/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: background #]
***/

#include "build.h"
#include "backgroundJob.h"
#include "backgroundJobImpl.h"

BEGIN_BOOMER_NAMESPACE()

//--

IBackgroundJob::~IBackgroundJob()
{}

void IBackgroundJob::cancel()
{
    m_cancelFlag = true;
}

//--

BackgroundScheduler::BackgroundScheduler()
{}

bool BackgroundScheduler::initialize(const IBaseCommandLine& commandline)
{
    return true;
}

void BackgroundScheduler::flush()
{
    ScopeTimer timer;

    for (;;)
    {
        m_threadBucketsLock.acquire();
        auto buckets = std::move(m_threadBuckets);
        m_threadBucketsLock.release();

        if (buckets.empty())
            break;

        TRACE_INFO("Canceling current background jobs...");

        for (auto* bucket : buckets.values())
        {
            auto lock = CreateLock(bucket->scheduledJobsLock);

            if (bucket->currentJob)
                bucket->currentJob->cancel();

            for (const auto& job : bucket->scheduledJobs)
                job->cancel();
            bucket->scheduledJobs.clear();

            bucket->canceledFlag = true;
        }

        TRACE_INFO("Waiting for background jobs to finish...");
        for (auto* bucket : buckets.values())
        {
            bucket->scheduledJobsSemaphore.release(10000);
            bucket->thread.close();
        }

        buckets.clearPtr();
    }

    TRACE_INFO("Finished background jobs in {}", timer);
}

BackgroundScheduler::ThreadBucket::ThreadBucket()
    : scheduledJobsSemaphore(0, INT_MAX)
{}

void BackgroundScheduler::schedule(IBackgroundJob* job, StringID bucketName)
{
    DEBUG_CHECK_RETURN_EX(job != nullptr, "Invalid job");
    DEBUG_CHECK_RETURN_EX(bucketName, "Invalid bucket");

    ThreadBucket* bucket = nullptr;

    // get bucket
    {
        auto lock = CreateLock(m_threadBucketsLock);

        bucket = m_threadBuckets.findSafe(bucketName, nullptr);
        if (!bucket)
        {
            bucket = new ThreadBucket();
            bucket->bucketName = bucketName;

            m_threadBuckets[bucketName] = bucket;

            ThreadSetup setup;
            setup.m_priority = ThreadPriority::BelowNormal;
            setup.m_name = "BackgroundJobThread";
            setup.m_stackSize = 1U << 20;
            setup.m_function = [this, bucket]() { runBucket(bucket); };

            bucket->thread.init(setup);
        }
    }

    // push job to bucket
    {
        {
            auto lock = CreateLock(bucket->scheduledJobsLock);
            bucket->scheduledJobs.pushBack(AddRef(job));
        }

        bucket->scheduledJobsSemaphore.release();
    }
}

//--

void BackgroundScheduler::runBucket(ThreadBucket* bucket)
{
    ScopeTimer timer;

    TRACE_INFO("Background thread for bucket '{}' started", bucket->bucketName);

    uint32_t numJobs = 0;
    double totalJobTime = 0.0;
    while (!bucket->canceledFlag)
    {
        bucket->scheduledJobsSemaphore.wait(500);

        {
            auto lock = CreateLock(bucket->scheduledJobsLock);
            if (!bucket->scheduledJobs.empty())
            {
                bucket->currentJob = bucket->scheduledJobs.back();
                bucket->scheduledJobs.popBack();
            }
        }

        if (bucket->currentJob)
        {
            PC_SCOPE_LVL0(BackgroundJob);
            bucket->currentJob->run();
            bucket->currentJob.reset();
        }
    }

    TRACE_INFO("Background thread for bucket '{}' finished after {}, processed {} jobs ({}), {}% utilization",
        bucket->bucketName, timer, numJobs, TimeInterval(totalJobTime), Prec((totalJobTime / timer.timeElapsed()) * 100.0, 2));
}

//--

void RunBackground(IBackgroundJob* job, StringID bucket /*= "default"_id*/)
{
    BackgroundScheduler::GetInstance().schedule(job, bucket);
}

//--

END_BOOMER_NAMESPACE()
