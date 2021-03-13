/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "core/containers/include/refCounted.h"
#include "core/system/include/semaphoreCounter.h"
#include "core/containers/include/queue.h"
#include "core/system/include/event.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//---

// generic background job
class GPU_API_COMMON_API IBaseBackgroundJob : public IReferencable
{
public:
	virtual ~IBaseBackgroundJob();

	//---

	/// describe job
	virtual void print(IFormatStream& f) const = 0;

	/// called on background processing thread
	virtual void process(volatile bool* vCancelFlag) = 0;

	//---

	/// check job completion result, returns true if job completed
	bool checkCompleted();

	/// wait for this job - can be called on only one thread, does not wait if job is already completed
	void waitUntilCompleted();

	//--

private:
	SpinLock m_lock;

	std::atomic<bool> m_completed = false;
	std::atomic<bool> m_cancel = false;
	std::atomic<Event*> m_completionEvent = nullptr;

	bool m_syncJob = false;
			
	void internalCancel();
	void internalProcess();

	friend class IBaseBackgroundQueue;
};

//---

// background queues
enum class BackgroundJobQueueType : uint8_t
{
	Background, // real background - do tasks from here if there's nothing else
	Current, // current queue, tasks intended to finish soon
};

//---

// generalized background job queue
// NOTE: this object MUST be deleted on render thread
class GPU_API_COMMON_API IBaseBackgroundQueue : public NoCopy
{
public:
	IBaseBackgroundQueue();
	virtual ~IBaseBackgroundQueue();

	//--

	bool initialize(const CommandLine& cmdLine);
	void stop();
	void update();

	//--

	void pushJob(BackgroundJobQueueType type, IBaseBackgroundJob* job);

	//--

	INLINE void pushNormalJob(IBaseBackgroundJob* job) { pushJob(BackgroundJobQueueType::Current, job); }
	INLINE void pushBackgroundJob(IBaseBackgroundJob* job) { pushJob(BackgroundJobQueueType::Background, job); }

	//--

protected:
	std::atomic<bool> m_exitingFlag = false;

	struct JobEntry
	{
		RTTI_DECLARE_POOL(POOL_API_RUNTIME);

	public:
		volatile bool cancelFlag = false;

		BackgroundJobPtr job;

		NativeTimePoint scheduledTime;
		NativeTimePoint startedTime;
		NativeTimePoint finishedTime;

		void print(IFormatStream& f) const;
	};

	SpinLock m_queueLock;
	Semaphore m_queueSemaphore;
	Queue<JobEntry*> m_queueNormal;
	Queue<JobEntry*> m_queueBackground;

	bool m_syncQueue = false;

	JobEntry* popNextJob(bool& outLoopFlag);
	JobEntry* popNextJob_NoLock();

	void processJob(JobEntry* entry);

	//--

	virtual bool createWorkerThreads(uint32_t requestedCount, uint32_t& outNumCreated) = 0;
	virtual void stopWorkerThreads() = 0;
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api)
