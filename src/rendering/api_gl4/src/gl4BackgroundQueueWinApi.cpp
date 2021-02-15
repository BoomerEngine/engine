/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: platform\winapi #]
* [# platform: winapi #]
***/

#include "build.h"
#include "gl4BackgroundQueueWinApi.h"
#include "base/system/include/thread.h"
#include "gl4ThreadWinApi.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

	        //--

			BackgroundQueueWinApi::BackgroundQueueWinApi(ThreadWinApi* owner)
				: m_owner(owner)
			{}

			bool BackgroundQueueWinApi::createWorkerThreads(uint32_t requestedCount, uint32_t& outNumCreated)
			{
				/*for (uint32_t i = 0; i < requestedCount; ++i)
				{
					if (auto* sharedContext = m_owner->createSharedContext())
					{
						auto* threadState = new ThreadState();

						base::ThreadSetup setup;
						setup.m_function = [this, threadState]() { threadFunc(threadState); };
						setup.m_name = "RenderingBackgroundJobThread";
						setup.m_stackSize = 256 << 10;
						setup.m_priority = base::ThreadPriority::BelowNormal;

						threadState->context = sharedContext;
						threadState->thread.init(setup);

						m_workerThreads.pushBack(threadState);
						outNumCreated += 1;
					}
				}*/

				return true;
			}

			void BackgroundQueueWinApi::stopWorkerThreads()
			{
				base::ScopeTimer timer;

				for (auto* state : m_workerThreads)
				{
					state->thread.close();

					delete state->context;
					state->context = nullptr;

					delete state;
				}

				TRACE_INFO("Stopped {} background processing threads in {}", m_workerThreads.size(), timer);
				m_workerThreads.clear();
			}

			void BackgroundQueueWinApi::threadFunc(ThreadState* state)
			{
				base::ScopeTimer timer;

				TRACE_INFO("Started background processing thread");

				state->context->activate();

				uint32_t jobCounter = 0;
				base::NativeTimeInterval jobBusyTimer;
				bool run = true;
				while (run)
				{
					if (auto* job = popNextJob(run))
					{
						const auto startTime = base::NativeTimePoint::Now();

						processJob(job);

						jobBusyTimer += startTime.timeTillNow();
						jobCounter += 1;
					}
				}

				state->context->deactivate();

				TRACE_INFO("Stopped background processing thread after {}, processed {} jobs in {} ({}% utilization)",
					timer, jobCounter, jobBusyTimer, Prec((jobBusyTimer.toSeconds() / timer.timeElapsed()) * 100.0, 2));
			}

			//--
	
		} // gl4
    } // api
} // rendering
