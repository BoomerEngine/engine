/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiCopyPool.h"

#include "base/system/include/spinLock.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/containers/include/queue.h"
#include "rendering/device/include/renderingDeviceApi.h"

namespace rendering
{
    namespace api
    {

        //---

        // a "copy engine" for OpenGL - mostly relies on persistently mapped staging buffers
        class RENDERING_API_COMMON_API IBaseCopyQueue : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_RUNTIME)

        public:
			IBaseCopyQueue(IBaseThread* owner, IBaseStagingPool* pool, ObjectRegistry* objects);
            ~IBaseCopyQueue();

			//--

			// push async copy job, tries to start copying data right away if there's a free staging area
			// NOTE: we can be called from unsafe area (outside render thread so sometimes when we can't allocate staging are we must wait)
			bool schedule(IBaseCopiableObject* ptr, const ResourceCopyRange& range, const ISourceDataProvider* sourceData, base::fibers::WaitCounter fence);

            //--

			// house keeping - signal finished copies, star new copies
			void update(Frame* frame);

			//--
            
        protected:
			IBaseThread* m_owner = nullptr;
			IBaseStagingPool* m_pool = nullptr;
			ObjectRegistry* m_objects = nullptr;

			//--
			
			struct Job : public base::IReferencable
			{
				RTTI_DECLARE_POOL(POOL_API_RUNTIME);

			public:
				INLINE Job() {};


				//--

				std::atomic<uint32_t> atomsIndex = 0;
				std::atomic<uint32_t> atomsLeft = 0;
				std::atomic<bool> canceled = false;

				base::Array<ResourceCopyAtom> atoms;
				ResourceCopyRange range;

				//--

				SourceDataProviderPtr sourceData; // source data to use for copying
				base::fibers::WaitCounter sourceFence; // fence to signal once copy is done
				void* sourceToken = nullptr; // token created by source data (internal loading state)
				bool sourceAsyncAtoms = false; // can atoms be initialized asynchronously

				//--

				base::NativeTimePoint timestamp; // when was the resource scheduled

				//--

				uint32_t stagingAreaSize = 0; // size of staging area for this resource
				uint32_t stagingAreaAlignment = 0; // alignment of staging area for this resource
				StagingArea stagingArea; // assigned when moved from pending to processing

				//--

				ObjectID id; // id of target object since it can go away

				//--
			};

			base::SpinLock m_pendingJobLock;
			base::Array<base::RefPtr<Job>> m_pendingJobs; // if everything goes fine use the queue
			
			base::SpinLock m_processingJobLock;
			base::Array<base::RefPtr<Job>> m_processingJobs;

			base::Array<base::RefPtr<Job>> m_tempJobList;

			void tryStartPendingJobs();
			bool tryStartJob(Job* job); // returns true if job is no longer pending
			void tryRunNextAtom(Job* job); // returns true if job is no longer pending

			void runAtom(Job* job, uint32_t atomIndex);
			void finishCompletedJobs(Frame* frame);

			void stop();

			//--
        };

        //---

    } // api
} // rendering