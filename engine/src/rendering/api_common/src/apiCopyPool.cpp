/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiCopyPool.h"
#include "base/system/include/thread.h"

namespace rendering
{
    namespace api
    {
		//--

		IBaseStagingPool::IBaseStagingPool(uint32_t size, uint32_t pageSize)
			: m_blockPoolTotalSize(size)
			, m_blockPoolPageSize(pageSize)
		{
			// initialize the storage, track blocks roughly in page size quanta, don't bother tracking smaller blocks directly (they are recovered though)
			DEBUG_CHECK(base::IsPow2(pageSize));
			DEBUG_CHECK(size % pageSize == 0);
			m_blockPool.setup(size, size / pageSize, pageSize);
			TRACE_INFO("GL: Allocated staging pool {}, {} blocks ({} page size)", MemSize(size), size / pageSize, MemSize(pageSize));
		}

		IBaseStagingPool::~IBaseStagingPool()
		{
		}

		//---

		StagingArea IBaseStagingPool::allocate(uint32_t size, base::StringView label)
		{
			DEBUG_CHECK_RETURN_V(size <= m_blockPoolTotalSize, StagingArea());

			MemoryBlock allocatedBlock;

			// allocate needed memory from block pool
			{
				auto lock = CreateLock(m_lock);

				const auto ret = m_blockPool.allocateBlock(size, m_blockPoolPageSize, allocatedBlock);
				if (ret != base::BlockAllocationResult::OK)
				{
					TRACE_SPAM("Allocation of staging block {} failed ({})", MemSize(size), label);
					return StagingArea();
				}

				const auto offset = base::BlockPool::GetBlockOffset(allocatedBlock);
				TRACE_SPAM("Allocated staging block {} @ {} for '{}', {} blocks allocated ({})", MemSize(size), offset, label, 
					m_blockPool.numAllocatedBlocks(), MemSize(m_blockPool.numAllocatedBytes()));
			}

			// yay, prepare staging area
			StagingArea ret;
			ret.block = allocatedBlock;
			ret.bufferOffset = base::BlockPool::GetBlockOffset(allocatedBlock);
			ret.bufferSize = size;
			ret.dataPtr = m_stagingAreaPtr + ret.bufferOffset;
			ret.apiPointer = m_apiPointer;
			ret.timestamp = base::NativeTimePoint::Now();

#ifndef BUILD_RELEASE
			ret.label = base::StringBuf(label);
#endif

			return ret;				
		}

		void IBaseStagingPool::free(const StagingArea& area)
		{
			DEBUG_CHECK_RETURN(!!area);
			DEBUG_CHECK_RETURN(area.apiPointer == m_apiPointer);
			DEBUG_CHECK_RETURN(area.bufferOffset <= m_blockPoolTotalSize);
			DEBUG_CHECK_RETURN(area.bufferOffset + area.bufferSize <= m_blockPoolTotalSize);

			{
				auto lock = CreateLock(m_lock);
				m_blockPool.freeBlock(area.block);

#ifndef BUILD_RELEASE
				base::StringView label = area.label;
#else
				base::StringView label;
#endif

				TRACE_SPAM("Freed staging block {} @ {} for '{}', {} blocks allocated ({})", MemSize(area.bufferSize), area.bufferOffset, label,
					m_blockPool.numAllocatedBlocks(), MemSize(m_blockPool.numAllocatedBytes()));
			}
		}

		//--	

    } // api
} // rendering
