/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "base/system/include/spinLock.h"
#include "base/containers/include/blockPool.h"

namespace rendering
{
    namespace api
    {

		//---

		// block of allocated staging area 
		struct StagingArea
		{
			PlatformPtr apiPointer;

			uint32_t bufferOffset = 0;
			uint32_t bufferSize = 0;
			void* dataPtr = nullptr;

			base::NativeTimePoint timestamp;

			MemoryBlock block; // for allocator

#ifndef BUILD_RELEASE
			base::StringBuf label;
#endif

			INLINE StagingArea() {};
			INLINE StagingArea(const StagingArea& other) = default;
			INLINE StagingArea& operator=(const StagingArea& other) = default;

			INLINE operator bool() const { return dataPtr != nullptr; }
		};

		///--

		// allocator for staging area, usually subclasses on the actual platform to create the heap/resources but core remain the same
		class RENDERING_API_COMMON_API IBaseStagingPool : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_API_RUNTIME)

		public:
			IBaseStagingPool(uint32_t size, uint32_t pageSize);
			virtual ~IBaseStagingPool();

			//---

			// get number of allocated staging blocks
			INLINE uint32_t numAllocatedBlocks() const { return m_blockPool.numAllocatedBlocks(); }

			// get number of allocated staging bytes
			INLINE uint32_t numAllocatedBytes() const { return m_blockPool.numAllocatedBytes(); }

			//---

			/// allocate a block of staging memory, fails if the block is not a viable
			/// NOTE: safe to call from ClientApi as the staging buffer is always preallocated
			StagingArea allocate(uint32_t size, base::StringView label);

			/// return staging area to pool (usually only happens after frame ends)
			void free(const StagingArea& area);

			//---

		private:
			base::SpinLock m_lock;

			base::BlockPool m_blockPool;
			uint32_t m_blockPoolTotalSize;
			uint32_t m_blockPoolPageSize;

			uint8_t* m_stagingAreaPtr = nullptr; // persistently mapped on all platform

			PlatformPtr m_apiPointer;
		};

        //---

    } // gl4
} // rendering