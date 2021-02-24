/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "base/containers/include/refCounted.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/containers/include/queue.h"
#include "base/system/include/event.h"

BEGIN_BOOMER_NAMESPACE(rendering::api)

//---

// generic background job
class RENDERING_API_COMMON_API IBaseBackgroundJob : public base::IReferencable
{
public:
	virtual ~IBaseBackgroundJob();

	//---

	/// describe job
	virtual void print(base::IFormatStream& f) const = 0;

	/// called on background processing thread
	virtual void process(volatile bool* vCancelFlag) = 0;

	//---

	/// check job completion result, returns true if job completed
	bool checkCompleted();

	/// wait for this job - can be called on only one thread, does not wait if job is already completed
	void waitUntilCompleted();

	//--

private:
	base::SpinLock m_lock;

	std::atomic<bool> m_completed = false;
	std::atomic<bool> m_cancel = false;
	std::atomic<base::Event*> m_completionEvent = nullptr;

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
class RENDERING_API_COMMON_API IBaseBackgroundQueue : public base::NoCopy 
{
public:
	IBaseBackgroundQueue();
	virtual ~IBaseBackgroundQueue();

	//--

	bool initialize(const base::app::CommandLine& cmdLine);
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

		base::NativeTimePoint scheduledTime;
		base::NativeTimePoint startedTime;
		base::NativeTimePoint finishedTime;

		void print(base::IFormatStream& f) const;
	};

	base::SpinLock m_queueLock;
	base::Semaphore m_queueSemaphore;
	base::Queue<JobEntry*> m_queueNormal;
	base::Queue<JobEntry*> m_queueBackground;

	bool m_syncQueue = false;

	JobEntry* popNextJob(bool& outLoopFlag);
	JobEntry* popNextJob_NoLock();

	void processJob(JobEntry* entry);

	//--

	virtual bool createWorkerThreads(uint32_t requestedCount, uint32_t& outNumCreated) = 0;
	virtual void stopWorkerThreads() = 0;
};

//---

END_BOOMER_NAMESPACE(rendering::api)