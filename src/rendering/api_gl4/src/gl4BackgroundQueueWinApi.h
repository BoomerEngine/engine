/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: platform\winapi #]
* [# platform: winapi #]
***/

#pragma once

#include "rendering/api_common/include/apiBackgroundJobs.h"
#include "base/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::gl4)

//---

class ThreadWinApi;
struct ThreadSharedContextWinApi;

class BackgroundQueueWinApi : public IBaseBackgroundQueue
{
public:
	BackgroundQueueWinApi(ThreadWinApi* owner);

	virtual bool createWorkerThreads(uint32_t requestedCount, uint32_t& outNumCreated) override final;
	virtual void stopWorkerThreads() override final;

private:
	ThreadWinApi* m_owner = nullptr;

	struct ThreadState
	{
		base::Thread thread;
		ThreadSharedContextWinApi* context = nullptr;
	};

	base::Array<ThreadState*> m_workerThreads;

	void threadFunc(ThreadState* state);
};

//---

END_BOOMER_NAMESPACE(rendering::api::gl4)