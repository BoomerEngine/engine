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
#include "base/containers/include/blockPool.h"

namespace rendering
{
    namespace gl4
    {

		//---

		// block of allocated staging area 
		struct DeviceCopyStagingArea
		{
			GLuint glBuffer = 0;
			uint32_t bufferOffset = 0;
			uint32_t bufferSize = 0;
			void* dataPtr = nullptr;

			base::NativeTimePoint timestamp;

			MemoryBlock block; // for allocator

#ifndef BUILD_RELEASE
			base::StringBuf label;
#endif

			INLINE DeviceCopyStagingArea() {};
			INLINE DeviceCopyStagingArea(const DeviceCopyStagingArea& other) = default;
			INLINE DeviceCopyStagingArea& operator=(const DeviceCopyStagingArea& other) = default;

			INLINE operator bool() const { return dataPtr != nullptr; }
		};

		// allocator for staging area
		class DeviceCopyStagingPool : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_API_RUNTIME)

		public:
			DeviceCopyStagingPool(Device* drv, uint32_t size, uint32_t pageSize);
			~DeviceCopyStagingPool();

			//---

			// get number of allocated staging blocks
			INLINE uint32_t numAllocatedBlocks() const { return m_blockPool.numAllocatedBlocks(); }

			// get number of allocated staging bytes
			INLINE uint32_t numAllocatedBytes() const { return m_blockPool.numAllocatedBytes(); }

			//---

			/// allocate a block of staging memory, fails if the block is not a viable
			/// NOTE: safe to call from ClientApi as the staging buffer is always preallocated
			DeviceCopyStagingArea allocate(uint32_t size, base::StringView label);

			/// return staging area to pool (usually only happens after frame ends)
			void free(const DeviceCopyStagingArea& area);

			//---

			/// flush memory writers to specific area
			ResolvedBufferView flushWrites(const DeviceCopyStagingArea& area);

		private:
			base::SpinLock m_lock;

			base::BlockPool m_blockPool;
			uint32_t m_blockPoolTotalSize;
			uint32_t m_blockPoolPageSize;

			uint8_t* m_stagingAreaPtr = nullptr;;

			GLuint m_glBuffer = 0;
		};

        //---

        // a "copy engine" for OpenGL - mostly relies on persistently mapped staging buffers
        class DeviceCopyQueue : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_RUNTIME)

        public:
			DeviceCopyQueue(Device* drv, DeviceCopyStagingPool* pool, ObjectRegistry* objects);
            ~DeviceCopyQueue();

			//--

			// push async copy job, tries to start copying data right away if there's a free staging area
			// NOTE: we can be called from unsafe area (outside render thread so sometimes when we can't allocate staging are we must wait)
			bool scheduleAsync_ClientApi(Object* ptr, const ResourceCopyRange& range, const ISourceDataProvider* sourceData, base::fibers::WaitCounter fence);

            //--

			// house keeping - signal finished copies, star new copies
			void update(Frame* frame);

			//--
            
        protected:
			DeviceCopyStagingPool* m_pool;
			ObjectRegistry* m_objects;

			//--
			
			struct CopyContext : public base::IReferencable
			{
				ObjectID id; // id of target object since it can go away				

				SourceDataProviderPtr sourceData;
				base::fibers::WaitCounter fence;
				base::NativeTimePoint timestamp;

				uint32_t stagingAreaSize = 0;
				DeviceCopyStagingArea stagingArea; // assigned when moved from pending to processing

				std::atomic<uint32_t> jobsLeft = 0;
			};

			struct Job : public base::IReferencable
			{
				RTTI_DECLARE_POOL(POOL_API_RUNTIME);

			public:
				INLINE Job() {};

				ResourceCopyRange range;
				uint32_t stagingAreaOffset = 0;

				base::RefPtr<CopyContext> m_context;

				void print(base::IFormatStream& f) const;
			};

			base::SpinLock m_pendingJobLock;
			base::Queue<base::RefPtr<Job>> m_pendingJobs;

			base::SpinLock m_processingJobLock;
			base::Array<base::RefPtr<Job>> m_processingJobs;

			base::Array<base::RefPtr<Job>> m_tempJobList;

			void tryStartPendingJobs();
			void finishCompletedJobs(Frame* frame);

			void stop();

			//--
        };

        //---

    } // gl4
} // rendering