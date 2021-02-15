/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiBackgroundJobs.h"
#include "base/system/include/thread.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

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
					base::Thread thread;
				};

				base::Array<ThreadState*> m_workerThreads;

				void threadFunc(ThreadState* state);
			};

			//---

		} // dx11
    } // api
} // rendering