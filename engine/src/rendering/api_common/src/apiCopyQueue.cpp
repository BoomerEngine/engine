/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiCopyQueue.h"
#include "apiObject.h"
#include "apiObjectRegistry.h"
#include "apiBuffer.h"
#include "apiImage.h"

#include "base/system/include/thread.h"
#include "rendering/device/include/renderingResources.h"

namespace rendering
{
    namespace api
    {
		//--

		IBaseCopyQueue::IBaseCopyQueue(IBaseThread* owner, ObjectRegistry* objects)
			: m_objects(objects)
			, m_owner(owner)
		{
			ASSERT(m_owner != nullptr);
			ASSERT(m_objects != nullptr);
		}

		IBaseCopyQueue::~IBaseCopyQueue()
		{}

		//--

		IBaseCopyQueueStagingArea::~IBaseCopyQueueStagingArea()
		{}

		//--	
		
		IBaseCopyQueueWithStaging::IBaseCopyQueueWithStaging(IBaseThread* owner, ObjectRegistry* objects)
			: IBaseCopyQueue(owner, objects)
		{
			static const uint32_t NUM_INITIAL_JOBS = 1024;

			m_pendingJobs.reserve(NUM_INITIAL_JOBS);
			m_processingJobs.reserve(NUM_INITIAL_JOBS);
		}

		IBaseCopyQueueWithStaging::~IBaseCopyQueueWithStaging()
		{
		}


		bool IBaseCopyQueueWithStaging::schedule(IBaseCopiableObject* ptr, const ISourceDataProvider* sourceData)
		{
			DEBUG_CHECK_RETURN_V(ptr != nullptr, false);
			DEBUG_CHECK_RETURN_V(sourceData != nullptr, false);

			auto* copiable = ptr->toCopiable();
			DEBUG_CHECK_RETURN_EX_V(copiable != nullptr, "Object is not copiable", nullptr);

			// create job
			auto job = base::RefNew<Job>();
			job->id = ptr->handle();
			job->sourceData = AddRef(sourceData);
			job->scheduledTimestamp = base::NativeTimePoint::Now();

			// ask the object for atoms
			copiable->computeStagingRequirements(job->stagingAtoms);

			// add to list of pending copy jobs
			{
				auto lock = base::CreateLock(m_processingJobLock);
				m_pendingJobs.pushBack(job);
			}

			// at least one job was added, try to start it
			tryStartPendingJobs();
			return true;
		}

		void IBaseCopyQueueWithStaging::update(uint64_t frameIndex)
		{
			finishCompletedJobs(frameIndex);
			tryStartPendingJobs();
		}

		void IBaseCopyQueueWithStaging::stop()
		{
			base::ScopeTimer timer;

			// remove any jobs from the list of jobs
			{
				auto lock = CreateLock(m_pendingJobLock);

				if (!m_pendingJobs.empty())
				{
					TRACE_INFO("There are {} scheduled copy jogs, canceling", m_pendingJobs.size());
					m_pendingJobs.clear();
				}
			}

			// wait for existing jobs to finish
			while (1)
			{
				{
					auto lock = CreateLock(m_processingJobLock);
					if (m_processingJobs.empty())
						break;

					TRACE_INFO("There are still {} jobs in processing list", m_processingJobs.size());

					for (const auto& job : m_processingJobs)
						job->canceled.exchange(true); // prevent length operation if possible

					base::Sleep(100);
				}
			}

			TRACE_INFO("Copy queue purged in {}", timer);
		}

		bool IBaseCopyQueueWithStaging::tryStartJob(Job* job)
		{
			// try to allocate staging memory/resource
			auto stagingArea = tryAllocateStagingForJob(*job);
			if (!stagingArea)
				return false;

			// yay, start job
			job->allocatedTimestamp.resetToNow();
			job->stagingArea = stagingArea;

			// run the copy on fiber
			RunFiber("AsyncResourceInit") << [this, job](FIBER_FUNC)
			{
				job->writeStartedTimestamp.resetToNow();

				copyJobIntoStagingArea(*job);

				job->writeFinishedTimestamp.resetToNow();
				job->finished.exchange(true);
			};

			// job started
			return true;
		}

		void IBaseCopyQueueWithStaging::tryStartPendingJobs()
		{
			auto lock = CreateLock(m_pendingJobLock);

			// process the queue
			uint32_t numStaredJobs = 0;
			{
				uint32_t index = 0;
				while (index < m_pendingJobs.size())
				{
					const auto& job = m_pendingJobs[index];
					if (tryStartJob(job))
					{
						{
							auto lock2 = CreateLock(m_processingJobLock);
							m_processingJobs.pushBack(job);
						}

						m_pendingJobs.erase(index);
						numStaredJobs += 1;
					}
					else
					{
						++index;
					}
				}
			}

			if (numStaredJobs > 0)
			{
				TRACE_SPAM("started {} async copy jobs, {} pending, {} processing", numStaredJobs, m_pendingJobs.size(), m_processingJobs.size());
			}
		}

		void IBaseCopyQueueWithStaging::finishCompletedJobs(uint64_t frameIndex)
		{
			base::InplaceArray<base::RefPtr<Job>, 256> finishedJobs;

			// look at the job list and extract ones completed
			{
				auto lock = CreateLock(m_processingJobLock);
				for (auto i : m_processingJobs.indexRange().reversed())
				{
					const auto& job = m_processingJobs[i];
					if (job->finished)
					{
						finishedJobs.pushBack(job);
						m_processingJobs.eraseUnordered(i);
					}
				}
			}

			// finally issue copies to target resources (if they still exist that is as they might have been deleted during the sourceData pulling)
			for (const auto& job : finishedJobs)
			{
				// find target object (might have been deleted)
				if (auto* obj = m_objects->resolveStatic(job->id))
				{
					auto* copiable = obj->toCopiable();
					ASSERT(copiable != nullptr);

					// flush data
					job->copyStartedTimestamp.resetToNow();
					flushStagingArea(job->stagingArea);

					// initialize the target resource from the staging data that was not filled by job
					copiable->initializeFromStaging(job->stagingArea);
					job->copyFinishedTimestamp.resetToNow();
				}
				else
				{
					TRACE_WARNING("Target object jof async copy job {} lost before copy finished", *job);
				}

				// signal fence now to indicate that we copied data to the GPU side
				job->sourceData->notifyFinshed(true);

				// release the staging area
				releaseStagingArea(job->stagingArea);
				job->stagingArea = nullptr;

				// print final stats
				{
					TRACE_SPAM("Finished copy job '{}' ({} atom)", *job, job->stagingAtoms.size());
					TRACE_SPAM("  Time to allocation: {}", (job->allocatedTimestamp - job->scheduledTimestamp));
					TRACE_SPAM("  Time to write start: {}", (job->writeStartedTimestamp - job->allocatedTimestamp));
					TRACE_SPAM("  Time to write finish: {}", (job->writeFinishedTimestamp - job->writeStartedTimestamp));
					TRACE_SPAM("  Time to copy started: {}", (job->copyStartedTimestamp - job->writeFinishedTimestamp));
					TRACE_SPAM("  Time to copy finished: {}", (job->copyFinishedTimestamp - job->copyStartedTimestamp));
					TRACE_SPAM("  Time to now: {}", job->scheduledTimestamp.timeTillNow());
				}
			}
		}		

		//--

    } // gl4
} // rendering
