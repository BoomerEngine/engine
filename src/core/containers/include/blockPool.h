/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: memory #]
***/

#pragma once

#include "sortedArray.h"
#include "core/memory/include/structurePool.h"

BEGIN_BOOMER_NAMESPACE()

/// memory allocation result
enum class BlockAllocationResult : uint8_t
{
	OK, // memory was allocated with no problems
	OutOfMemory, // we don't have enough memory to service the request (and it's not a fragmentation problem)
	OutOfBlocks, // we don't have a free block for the new allocation (we DO have memory though)
	FragmentationError, // we do have memory and a block but the memory is fragmented, should trigger defragmentation request
	InvalidBlock, // block to allocate is invalid
};

/// defragmentation operation request
struct BlockToDefrag
{
	uint32_t sourceOffset = 0;
	uint32_t destinationOffset = 0;
	uint32_t size = 0;
	MemoryBlock block = INVALID_MEMORY_BLOCK;
};

/// memory statistics
struct BlockPoolStats
{
	uint64_t totalPoolSize = 0; // size of the whole pool
	uint32_t totalBlockCount = 0; // total number of blocks this pool can service

	uint64_t currentAllocatedBytes = 0; // number of bytes (with alignment) currently allocated from the pool
	uint64_t maxAllocatedBytes = 0; // maximum amount of bytes ever allocated from the pool
	uint32_t currentAllocatedBlocks = 0; // number of blocks currently allocated from the pool
	uint32_t maxAllocatedBlocks = 0; // maximum amount of blocks ever allocated from the pool
	uint64_t wastedBytes = 0; // bytes that were wasted for alignment and small (unallocatable) blocks

	uint64_t defragPendingBytes = 0; // number of bytes being defragmented ATM (blocks that move was requested but not yet signaled)
	uint32_t defragPendingBlocks = 0; // number of blocks being defragmented ATM (blocks that move was requested but not yet signaled)

	uint64_t largestFreeBlockBytes = 0; // size of the largest free block this pool
	uint32_t freeBlockCount = 0; // number of free blocks
};

/// block memory allocator with support for defragmentation
/// NOTE: this is used only by drivers that can manage their own memory
class CORE_CONTAINERS_API BlockPool
{
public:
	BlockPool();
	~BlockPool();
		
	///---

	/// number of allocated bytes
	INLINE uint64_t numAllocatedBytes() const { return m_numAllocatedBytes; };

    /// number of allocated blocks
    INLINE uint32_t numAllocatedBlocks() const { return m_numAllocatedBlocks; }

	///---

	/// initialize allocator with given size and number of block, resets current content
	void setup(uint32_t memorySize, uint32_t maxBlocks, uint32_t minFreeBlockSize);

	/// get current statistics
	void stats(BlockPoolStats& outStats) const;

	///---

	/// allocate block in the allocator
	BlockAllocationResult allocateBlock(uint32_t size, uint32_t alignment, MemoryBlock& outBlock);

	/// free previously allocated block
	void freeBlock(MemoryBlock id);

	///---

	/// defragmentation interface - get list of blocks to copy
	/// the returned blocks are internally marked as "DO NOT DELETE" for the duration of the move
	/// the request can be throttled by specifying the maximum amount of data and/or blocks that we want to move 
	/// NOTE: we MUST call the signalBlockMoved for EACH block returned here or else we will leak the memory permanently
	/// NOTE: the calls can overlap (you may call requestBlocksToMove again before calling signalBlockMoved on all previously returned blocks)
	void requestBlocksToMove(uint64_t maxBytesToMove, uint32_t maxBlocksToMove, Array<BlockToDefrag>& outBlocks);

	/// signal that the block previously returned by requestBlocksToMove was moved
	/// blocks can be signaled one by one
	void signalBlockMoved(MemoryBlock id);

	///---

	/// get offset of the given block, block must be allocated
	INLINE static uint64_t GetBlockOffset(MemoryBlock block)
	{
		auto nativeBlock  = (const Block*)block;
		ASSERT(nativeBlock->allocated);
		return nativeBlock->offset;
	}

	/// get size of the given block, block must be allocated
	INLINE static uint64_t GetBlockSize(MemoryBlock block)
	{
		auto nativeBlock  = (const Block*)block;
		ASSERT(nativeBlock->allocated);
		return nativeBlock->size;
	}

private:
	struct Block
	{
		uint8_t allocated : 1;
		uint8_t defragSource : 1;
		uint8_t defragDestination : 1;
		uint64_t offset = 0;
		uint64_t size = 0;

		Block* next = nullptr;
		Block* prev = nullptr;

		INLINE Block()
			: allocated(0)
			, defragSource(0)
			, defragDestination(0)
		{}

		INLINE void unlink()
		{
			if (next)
                next->prev = prev;
			if (prev)
                prev->next = next;
            next = nullptr;
            prev = nullptr;
		}

		INLINE void link(Block*& head)
		{
            prev = nullptr;
            next = head;
			if (head)
				head->prev = this;
			head = this;
		}

		INLINE void linkAfter(Block* after)
		{
            prev = after;
            next = after->next;

			if (after->next)
				after->next->prev = this;
			after->next = this;
		}

	};

	struct FreeBlock
	{
		uint64_t m_freeSize;
		Block* m_block;

		INLINE FreeBlock()
			: m_block(nullptr)
			, m_freeSize(0)
		{}

		INLINE FreeBlock(Block* block)
			: m_block(block)
			, m_freeSize(block->size)
		{}

		INLINE FreeBlock(uint64_t freeSize)
			: m_block(nullptr)
			, m_freeSize(freeSize)
		{}

		INLINE bool operator==(const FreeBlock& other) const
		{
			return (m_freeSize == other.m_freeSize) && (m_block == other.m_block);
		}

		INLINE bool operator<(const FreeBlock& other) const
		{
			if (m_freeSize != other.m_freeSize)
				return m_freeSize < other.m_freeSize;

			if (other.m_block && m_block)
				return m_block->offset < other.m_block->offset;

			return false;
		}

		INLINE bool canAllocate(uint32_t size, uint32_t alignment) const
		{
			auto alignedOffset  = Align(m_block->offset, range_cast<uint64_t>(alignment));
			auto endOffset  = m_block->offset + m_block->size;
			return alignedOffset + size <= endOffset;
		}
	};

	mem::StructurePool<Block> m_blockPool; // linked list of all blocks in the pool, very slow to visit directly
	Block* m_blockList = nullptr; // list of all blocks ordered by the offset

	uint64_t m_totalSize = 0; // total size of memory we manage
	uint32_t m_totalBlocks = 0; // total allowed allocated blocks
	uint32_t m_minFreeBlockSize = 0; // free blocks below this size are not tracked and cannot be allocated

	SortedArray<FreeBlock> m_freeBlocks; // free blocks sorted by their size (larges to smallest)

	int firstBestFreeBlock(uint32_t size, uint32_t alignment) const;
	Block* extractUsedBlock(Block* freeBlock, uint64_t splitStart, uint64_t splitEnd);

	void validate() const;

	///--

	uint64_t m_numAllocatedBytes = 0;
	uint64_t m_maxAllocatedBytes = 0;
	uint32_t m_numAllocatedBlocks = 0;
	uint32_t m_maxAllocatedBlocks = 0;

};

END_BOOMER_NAMESPACE()

 