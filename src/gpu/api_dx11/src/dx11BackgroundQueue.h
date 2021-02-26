/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/api_common/include/apiBackgroundJobs.h"
#include "core/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::dx11)

//---

class ThreadWinApi;

class BackgroundQueue : public IBaseBackgroundQueue
{
public:
	BackgroundQueue(Thread* owner);

	virtual bool createWorkerThreads(uint32_t requestedCount, uint32_t& outNumCreated) override final;
	virtual void stopWorkerThreads() override final;

private:
	Thread* m_owner = nullptr;

	struct ThreadState
	{
		boomer::Thread thread;
	};

	Array<ThreadState*> m_workerThreads;

	void threadFunc(ThreadState* state);
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api::dx11)
