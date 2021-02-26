/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiBackgroundJobs.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

IBaseBackgroundJob::~IBaseBackgroundJob()
{
 	delete m_completionEvent;
	m_completionEvent = nullptr;
}

bool IBaseBackgroundJob::checkCompleted()
{
	return m_completed;
}

void IBaseBackgroundJob::waitUntilCompleted()
{
	if (!m_completed.load())
	{
		if (m_syncJob)
		{
			internalProcess();
		}
		else
		{
			// ensure mutex exists
			{
				auto lock = CreateLock(m_lock);
				if (!m_completed.load())
				{
					m_completionEvent = new Event(true);
				}
			}

			// wait for the event
			if (auto* evt = m_completionEvent.load())
			{
				ScopeTimer timer;
				evt->waitInfinite();
				TRACE_INFO("Waited {} for completion of background job {}", timer, IndirectPrint(this));
			}
		}
	}
}

void IBaseBackgroundJob::internalCancel()
{
	m_cancel.exchange(true);
}

void IBaseBackgroundJob::internalProcess()
{
	if (!m_cancel.load())
		process((volatile bool*) &m_cancel);

	if (!m_syncJob)
	{
		auto lock = CreateLock(m_lock);

		if (auto* evt = m_completionEvent.load())
			evt->trigger();
	}

	m_completed.store(true);
}

//--

IBaseBackgroundQueue::IBaseBackgroundQueue()
	: m_queueSemaphore(0, 1 << 20)
{}

IBaseBackgroundQueue::~IBaseBackgroundQueue()
{
	ASSERT(m_queueNormal.empty());
	ASSERT(m_queueBackground.empty());
}

//--

ConfigProperty<uint32_t> cvBackgroundQueueThreads("Rendering", "BackgroundJobThreadCount", 1);
ConfigProperty<uint32_t> cvBackgroundQueueTimeout("Rendering", "BackgroundQueueTimeout", 100);
ConfigProperty<uint32_t> cvBackgroundQueueSemaphoreWait("Rendering", "BackgroundQueueSemaphoreWait", 5);

bool IBaseBackgroundQueue::initialize(const app::CommandLine& cmdLine)
{
	const auto numThreadsRequested = cvBackgroundQueueThreads.get();

	uint32_t numThreadsCreated = 0;
	if (!createWorkerThreads(numThreadsRequested, numThreadsCreated))
	{
		TRACE_ERROR("Failed to create background worker threads");
		return false;
	}

	if (numThreadsCreated == 0)
	{
		TRACE_WARNING("No background threads created, queue will run in sync mode");
		m_syncQueue = true;
	}
	else
	{
		TRACE_INFO("Created {} background worker threads (of {} requested)", numThreadsRequested, numThreadsCreated);
		m_syncQueue = false;
	}

	m_queueNormal.reserve(1024);
	m_queueBackground.reserve(1024);

	return true;
}

void IBaseBackgroundQueue::stop()
{
	m_exitingFlag = true;

	{
		auto lock = CreateLock(m_queueLock);
		TRACE_INFO("Stopping background queue ({} normal job(s), {} background job(s))", m_queueNormal.size(), m_queueBackground.size());

		while (auto* job = popNextJob_NoLock())
		{
			job->job->internalCancel();
			job->cancelFlag = true;
			delete job;
		}
	}

	stopWorkerThreads();

	{
		auto lock = CreateLock(m_queueLock);
		ASSERT(m_queueNormal.empty());
		ASSERT(m_queueBackground.empty());
	}
}

void IBaseBackgroundQueue::update()
{
	if (!m_syncQueue)
		return;

	PC_SCOPE_LVL0(SyncRenderingJobs);

	const auto startTime = NativeTimePoint::Now();
	const auto timeoutTime = startTime + (cvBackgroundQueueTimeout.get() / 1000.0);

	// process all normal jobs, no stopping
	for (;;)
	{
		JobEntry* entry = nullptr;

		// get top entry from normal queue
		{
			auto lock = CreateLock(m_queueLock);
			if (m_queueNormal.empty())
				break;
			entry = m_queueNormal.top();
			m_queueNormal.pop();
		}

		// process it
		processJob(entry);
	}

	// process all background jobs
	while (!timeoutTime.reached())
	{
		JobEntry* entry = nullptr;

		// get top entry from normal queue
		{
			auto lock = CreateLock(m_queueLock);
			if (m_queueBackground.empty())
				break;
			entry = m_queueBackground.top();
			m_queueBackground.pop();
		}

		// process it
		processJob(entry);
	}
}

//--

void IBaseBackgroundQueue::JobEntry::print(IFormatStream& f) const
{
	job->print(f);
}

void IBaseBackgroundQueue::processJob(JobEntry* entry)
{
	entry->startedTime.resetToNow();
	entry->job->internalProcess();
	entry->finishedTime.resetToNow();

	const auto waitTime = entry->finishedTime - entry->scheduledTime;
	const auto processingTime = entry->finishedTime - entry->startedTime;
	TRACE_INFO("Background job {} finished after {} (scheduled {} ago)", IndirectPrint(entry), processingTime, waitTime);

	delete entry;
}

void IBaseBackgroundQueue::pushJob(BackgroundJobQueueType type, IBaseBackgroundJob* job)
{
	{
		auto lock = CreateLock(m_queueLock);
		DEBUG_CHECK_RETURN_EX(!m_exitingFlag, "Queue is in existing state yet new jobs are being pushed, investigate");

		auto* entry = new JobEntry;
		entry->cancelFlag = false;
		entry->job = AddRef(job);
		entry->scheduledTime.resetToNow();

		job->m_syncJob = m_syncQueue;

		if (type == BackgroundJobQueueType::Background)
			m_queueBackground.push(entry);
		else if (type == BackgroundJobQueueType::Current)
			m_queueNormal.push(entry);
	}

	m_queueSemaphore.release(1);
}

IBaseBackgroundQueue::JobEntry* IBaseBackgroundQueue::popNextJob(bool& outLoopFlag)
{
	m_queueSemaphore.wait(cvBackgroundQueueSemaphoreWait.get());

	if (m_exitingFlag)
	{
		outLoopFlag = false;
		return nullptr;
	}

	{
		auto lock = CreateLock(m_queueLock);
		return popNextJob_NoLock();
	}
}

IBaseBackgroundQueue::JobEntry* IBaseBackgroundQueue::popNextJob_NoLock()
{
	IBaseBackgroundQueue::JobEntry* ret = nullptr;

	if (!m_queueNormal.empty())
	{
		ret = m_queueNormal.top();
		m_queueNormal.pop();
	}
	else if (!m_queueBackground.empty())
	{
		ret = m_queueBackground.top();
		m_queueBackground.pop();
	}

	if (ret)
		ret->startedTime.resetToNow();

	return ret;
}

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
