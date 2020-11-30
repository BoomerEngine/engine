/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiCopyPool.h"
#include "apiCopyQueue.h"
#include "apiObject.h"
#include "apiObjectRegistry.h"
#include "apiBuffer.h"
#include "apiImage.h"
#include "apiFrame.h"

#include "base/system/include/thread.h"
#include "rendering/device/include/renderingResources.h"

namespace rendering
{
    namespace api
    {

		//--	
		
		IBaseCopyQueue::IBaseCopyQueue(IBaseThread* owner, IBaseStagingPool* pool, ObjectRegistry* objects)
			: m_pool(pool)
			, m_objects(objects)
			, m_owner(owner)
		{
			ASSERT(m_pool != nullptr);
			ASSERT(m_owner != nullptr);
			ASSERT(m_objects != nullptr);

			static const uint32_t NUM_INITIAL_JOBS = 1024;

			m_pendingJobs.reserve(NUM_INITIAL_JOBS);
			m_processingJobs.reserve(NUM_INITIAL_JOBS);
			m_tempJobList.reserve(NUM_INITIAL_JOBS);
		}

		IBaseCopyQueue::~IBaseCopyQueue()
		{
			stop();
		}

		bool IBaseCopyQueue::schedule(IBaseCopiableObject* ptr, const ResourceCopyRange& range, const ISourceDataProvider* sourceData, base::fibers::WaitCounter fence)
		{
			DEBUG_CHECK_RETURN_V(ptr != nullptr, false);
			DEBUG_CHECK_RETURN_V(sourceData != nullptr, false);

			auto* copiable = ptr->toCopiable();
			DEBUG_CHECK_RETURN_EX_V(copiable != nullptr, "Object is not copiable", nullptr);

			// create job
			auto job = base::RefNew<Job>();
			job->id = ptr->handle();
			job->range = range;
			job->sourceFence = fence;
			job->sourceData = AddRef(sourceData);
			job->timestamp = base::NativeTimePoint::Now();

			// generate list of atoms (part of resources) that we want to copy
			base::Array<ResourceCopyAtom> atoms;
			if (!copiable->generateCopyAtoms(range, atoms, job->stagingAreaSize, job->stagingAreaAlignment))
			{
				TRACE_ERROR("Unable to create async copy job for object {} from '{}'", *ptr, sourceData->debugLabel());
				Fibers::GetInstance().signalCounter(fence);
				return false;
			}

			
			// ask the source to start preparing for copy
			if (!sourceData->openSourceData(range, job->sourceToken, job->sourceAsyncAtoms))
			{
				TRACE_ERROR("Unable to create async copy job for object {} from '{}' becase source could not be open", *ptr, sourceData->debugLabel());
				Fibers::GetInstance().signalCounter(fence);
				return false;
			}

			// at least one job was added, try to start it
			tryStartPendingJobs();
			return true;
		}

		void IBaseCopyQueue::update(Frame* frame)
		{
			finishCompletedJobs(frame);
			tryStartPendingJobs();
		}

		void IBaseCopyQueue::stop()
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
	

		void IBaseCopyQueue::tryRunNextAtom(Job* job)
		{
			auto atomIndex = job->atomsIndex++;
			ASSERT(atomIndex < job->atoms.size());

			RunFiber("AsyncCopyAtom") << [this, job, atomIndex](FIBER_FUNC)
			{
				runAtom(job, atomIndex);
			};
		}

		void IBaseCopyQueue::runAtom(Job* job, uint32_t atomIndex)
		{
			if (!job->canceled)
			{
				const auto& atom = job->atoms[atomIndex];

				{
					PC_SCOPE_LVL2(CopyAtom);
					void* destStagingArea = (uint8_t*)job->stagingArea.dataPtr + atom.stagingAreaOffset;
					job->sourceData->writeSourceData(destStagingArea, job->sourceToken, atom.copyElement);
				}

				// count atom as finished, this may start new one in the sync mode
				if (0 == --job->atomsLeft)
				{
					job->sourceData->closeSourceData(job->sourceToken);
					job->sourceToken = nullptr;
				}
				else
				{
					if (!job->sourceAsyncAtoms)
						tryRunNextAtom(job);
				}
			}
			else
			{
				if (0 != job->atomsLeft.exchange(0))
				{
					job->sourceData->closeSourceData(job->sourceToken);
					job->sourceToken = nullptr;
				}
			}
		}

		bool IBaseCopyQueue::tryStartJob(Job* job)
		{
			// is the job ready ?
			const auto ready = job->sourceData->checkSourceDataReady(job->sourceToken);
			if (ready == ISourceDataProvider::ReadyStatus::NotReady)
				return false;

			// job source failed, finish the job as well
			if (ready == ISourceDataProvider::ReadyStatus::Failed)
			{
				job->sourceData->closeSourceData(job->sourceToken);
				job->sourceToken = nullptr;

				TRACE_ERROR("Unable to open source data for object {} from '{}' becase source could not be open", job->id, job->sourceData->debugLabel());
				return true; // do not retry
			}

			// try to allocate memory
			job->stagingArea = m_pool->allocate(job->stagingAreaSize, job->sourceData->debugLabel());
			if (!job->stagingArea)
			{
				TRACE_SPAM("still no memory to start jobs with {} staging area, {} pending, {} processing, {} staging used in {} blocks",
					MemSize(job->stagingAreaSize), m_pendingJobs.size(), m_processingJobs.size(), MemSize(m_pool->numAllocatedBytes()), m_pool->numAllocatedBlocks());
				return false;
			}

			// create the fiber that will handle copying the data to the staging area
			// NOTE: this can happen outside of render thread as it's basically a memcpy
			if (job->sourceAsyncAtoms)
			{
				// start all atoms
				for (auto i : job->atoms.indexRange())
					runAtom(job, i);
			}
			else
			{
				// start first atom, rest will follow once first copy finished
				runAtom(job, 0);
			}

			// job started
			return true;
		}

		void IBaseCopyQueue::tryStartPendingJobs()
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
				TRACE_SPAM("started {} async copy jobs, {} pending, {} processing, {} staging used in {} blocks",
					numStaredJobs, m_pendingJobs.size(), m_processingJobs.size(), MemSize(m_pool->numAllocatedBytes()), m_pool->numAllocatedBlocks());
			}
		}

		void IBaseCopyQueue::finishCompletedJobs(Frame* frame)
		{
			// look at the job list and extract ones completed
			{
				m_tempJobList.reset();

				auto lock = CreateLock(m_processingJobLock);
				for (auto i : m_processingJobs.indexRange().reversed())
				{
					const auto& job = m_processingJobs[i];
					if (job->atomsLeft == 0)
					{
						TRACE_SPAM("Discovered finished job '{}' all {} atoms finished, {} since scheduling", *job, job->timestamp.timeTillNow());

						m_tempJobList.pushBack(job);
						m_processingJobs.eraseUnordered(i);
					}
				}
			}

			// finally issue copies to target resources (if they still exist that is as they might have been deleted during the sourceData pulling)
			for (const auto& job : m_tempJobList)
			{
				// find target object (might have been deleted)
				if (auto* obj = m_objects->resolveStatic(job->id))
				{
					auto* copiable = obj->toCopiable();
					ASSERT(copiable != nullptr);

					copiable->applyCopyAtoms(job->atoms, frame, job->stagingArea);
				}
				else
				{
					TRACE_WARNING("Target object jof async copy job {} lost before copy finished", *job);
				}

				// make sure the staging area is reused at the end of current frame
				auto area = job->stagingArea;
				frame->registerCompletionCallback([this, area]()
					{
						m_pool->free(area);
					});

				// signal fence now to indicate that we copied data to the GPU side
				Fibers::GetInstance().signalCounter(job->sourceFence, 1);
			}
		}		

		//--

    } // gl4
} // rendering
