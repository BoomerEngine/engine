/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "base/system/include/spinLock.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/containers/include/queue.h"
#include "rendering/device/include/renderingDeviceApi.h"

namespace rendering
{
    namespace api
    {

        //---

        // a "copy engine" interface, used for asynchronous (non-blocking) resource initialization
		class RENDERING_API_COMMON_API IBaseCopyQueue : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_API_RUNTIME)

		public:
			IBaseCopyQueue(IBaseThread* owner, ObjectRegistry* objects);
			virtual ~IBaseCopyQueue();

			//--

			// push async copy (more like resource initialization) job
			// NOTE: we can be called from unsafe area (outside render thread so sometimes when we can't allocate staging are we must wait)
			virtual bool schedule(IBaseCopiableObject* ptr, const ISourceDataProvider* sourceData) = 0;

			//--

			// house keeping, called on render thread before processing next batch of commands
			// NOTE: may be called multiple times within one frame
			virtual void update(uint64_t frameIndex) = 0;

			//--

		protected:
			IBaseThread* m_owner = nullptr;
			ObjectRegistry* m_objects = nullptr;
		};

		//--

		class RENDERING_API_COMMON_API IBaseCopyQueueStagingArea : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_API_RUNTIME);

		public:
			virtual ~IBaseCopyQueueStagingArea();
		};

		//--

		// staging area based copy queue - useful for all APIs where there's a "CopyFromBuffer" functionality (so all except DirectX11)
		class RENDERING_API_COMMON_API IBaseCopyQueueWithStaging : public IBaseCopyQueue
		{
		public:
			IBaseCopyQueueWithStaging(IBaseThread* owner, ObjectRegistry* objects);
			virtual ~IBaseCopyQueueWithStaging();

			//--

			virtual bool schedule(IBaseCopiableObject* ptr, const ISourceDataProvider* sourceData) override final;

			virtual void update(uint64_t frameIndex) override;

			//--

		protected:
			

			struct Job : public base::IReferencable
			{
				RTTI_DECLARE_POOL(POOL_API_RUNTIME);

			public:
				INLINE Job() {};

				//--

				// id of target object since it can go away
				ObjectID id; 

				// "atoms" to copy - sub-parts of the original resource
				base::Array<StagingAtom> stagingAtoms;

				// staging resource/area assigned to the resource
				IBaseCopyQueueStagingArea* stagingArea = nullptr;

				// source data to use for copying
				SourceDataProviderPtr sourceData;

				// time tracking...
				base::NativeTimePoint scheduledTimestamp; 
				base::NativeTimePoint allocatedTimestamp;
				base::NativeTimePoint writeStartedTimestamp;
				base::NativeTimePoint writeFinishedTimestamp;
				base::NativeTimePoint copyStartedTimestamp;
				base::NativeTimePoint copyFinishedTimestamp;

				// task state
				std::atomic<bool> canceled = false; //  used only when whole system is being closed
				std::atomic<bool> finished = false;
			};

			base::SpinLock m_pendingJobLock;
			base::Array<base::RefPtr<Job>> m_pendingJobs; // if everything goes fine use the queue

			base::SpinLock m_processingJobLock;
			base::Array<base::RefPtr<Job>> m_processingJobs;

			//--

			void tryStartPendingJobs();
			bool tryStartJob(Job* job); // returns true if job is no longer pending
			void finishCompletedJobs(uint64_t frameIndex);

			void stop();

			//--

			// given a job try to allocate necessary (platform dependent) staging resources for it
			virtual IBaseCopyQueueStagingArea* tryAllocateStagingForJob(const Job& job) = 0;

			// flush staging area (make data visible to gpu)
			virtual void flushStagingArea(IBaseCopyQueueStagingArea* area) = 0;

			// release staging area
			virtual void releaseStagingArea(IBaseCopyQueueStagingArea* area) = 0;

			// process job - copy user content from SourceDataProvider -> StagingArea
			virtual void copyJobIntoStagingArea(const Job& job) = 0;

			//--
		};

        //---

    } // api
} // rendering