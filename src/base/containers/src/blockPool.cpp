/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: memory #]
*/

#include "build.h"

#include "blockPool.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

BlockPool::BlockPool()
{}

BlockPool::~BlockPool()
{
    ASSERT_EX(m_numAllocatedBlocks == 0, "Block allocator destroyed while there are still unfreed blocks allocated from it");
    if (nullptr != m_blockList)
    {
        ASSERT(m_blockList->prev == nullptr);
        ASSERT(m_blockList->next == nullptr);
        m_blockPool.free(m_blockList);
    }
}

void BlockPool::validate() const
{
    uint64_t pos = 0;
    const Block* prevBlock = nullptr;
    uint32_t numFreeBlocks = 0;
    uint64_t largestFreeBlock = 0;
    uint64_t numAllocatedBytes = 0;
    uint32_t numAllocatedBlocks = 0;
    auto cur  = m_blockList;
    while (cur)
    {
        ASSERT(cur->offset == pos);
        ASSERT(cur->prev == prevBlock);

        if (!cur->allocated)
        {
            ASSERT(!cur->prev || cur->prev->allocated);
            ASSERT(!cur->next || cur->next->allocated);
            numFreeBlocks += 1;
            largestFreeBlock = std::max(largestFreeBlock, cur->size);
        }
        else
        {
            numAllocatedBytes += cur->size;
            numAllocatedBlocks += 1;
        }

        pos += cur->size;
        prevBlock = cur;
        cur = cur->next;
    }

    ASSERT(pos == m_totalSize);
    ASSERT(numFreeBlocks >= m_freeBlocks.size());
    ASSERT(numAllocatedBlocks == m_numAllocatedBlocks);
    ASSERT(numAllocatedBytes == m_numAllocatedBytes);
    if (!m_freeBlocks.empty())
    {
        ASSERT(m_freeBlocks.back().m_freeSize == largestFreeBlock);
    }

    for (uint32_t i = 0; i < m_freeBlocks.size(); ++i)
    {
        if (i > 0)
        {
            ASSERT(m_freeBlocks[i - 1].m_freeSize <= m_freeBlocks[i].m_freeSize);
        }

        auto block  = m_freeBlocks[i].m_block;
        ASSERT(!block->allocated);
        ASSERT(block->size == m_freeBlocks[i].m_freeSize);
    }
}

void BlockPool::setup(uint32_t memorySize, uint32_t maxBlocks, uint32_t minFreeBlockSize)
{
    // reset the pool
    m_blockList = nullptr;
    m_totalSize = memorySize;
    m_totalBlocks = maxBlocks;
    m_minFreeBlockSize = minFreeBlockSize;
    //m_blockPool.preallocate(2 + maxBlocks + (maxBlocks-1)); // each allocated block may be separated by a free block

    // reset stats
    m_numAllocatedBytes = 0;
    m_maxAllocatedBytes = 0;
    m_numAllocatedBlocks = 0;
    m_maxAllocatedBlocks = 0;

    // create the root block
    auto rootBlock  = m_blockPool.create();
    rootBlock->offset = 0;
    rootBlock->size = m_totalSize;
    rootBlock->link(m_blockList);

    // insert the free block
    ASSERT(memorySize >= m_minFreeBlockSize);
    m_freeBlocks.insert(FreeBlock(rootBlock));
}

void BlockPool::stats(BlockPoolStats& outStats) const
{
    outStats.totalPoolSize = m_totalSize;
    outStats.totalBlockCount = m_totalBlocks;
    outStats.defragPendingBlocks = 0;
    outStats.defragPendingBytes = 0;
    outStats.currentAllocatedBytes = m_numAllocatedBytes;
    outStats.currentAllocatedBlocks = m_numAllocatedBlocks;
    outStats.maxAllocatedBytes = m_maxAllocatedBytes;
    outStats.maxAllocatedBlocks = m_maxAllocatedBlocks;
    outStats.freeBlockCount = m_freeBlocks.size();
    outStats.largestFreeBlockBytes = m_freeBlocks.empty() ? 0 : m_freeBlocks.back().m_freeSize;

    uint64_t numFreeBytes = 0;
    for (auto& freeBlock : m_freeBlocks)
        numFreeBytes += freeBlock.m_freeSize;

    outStats.wastedBytes = m_totalSize - m_numAllocatedBytes - numFreeBytes;
}

int BlockPool::firstBestFreeBlock(uint32_t size, uint32_t alignment) const
{
    // the list may be empty if we are fragmented as fuck
    if (m_freeBlocks.empty())
        return INDEX_NONE;

    // do a binary search to find the first block that has enough space to even try
    auto it  = std::lower_bound(m_freeBlocks.begin(), m_freeBlocks.end(), FreeBlock(size));
    while (it != m_freeBlocks.end())
    {
        // to an exact test to determine if we can allocate from given free block (including alignment requirements)
        if (it->canAllocate(size, alignment))
            return (it - m_freeBlocks.begin());
        ++it;
    }

    // no block found that can allocate given entry
	TRACE_ERROR("No free block found for size {}, alignment {} in {} currently free blocks:", size, alignment, m_freeBlocks.size());
	for (uint32_t i = 0; i < m_freeBlocks.size(); ++i)
	{
		auto& entry = m_freeBlocks[i];
		TRACE_INFO("FreeBlock[{}]: free size {}, block offset {}, block size {}", i, entry.m_freeSize, entry.m_block->offset, entry.m_block->size);
	}
    return INDEX_NONE;
}

BlockPool::Block* BlockPool::extractUsedBlock(Block* freeBlock, uint64_t splitStart, uint64_t splitEnd)
{
    // extract the initial free space into a separate block
    if (splitStart > freeBlock->offset)
    {
        // leave the current free block where it is just shrink it's size to the small piece in the front of the allocation that will be left
        auto originalSize  = freeBlock->size;
        freeBlock->size = splitStart - freeBlock->offset;

        // move rest of the memory to the new block that starts at the exact position of the split
        auto extraBlock  = m_blockPool.create();
        extraBlock->allocated = 0;
        extraBlock->offset = splitStart;
        extraBlock->size = originalSize - freeBlock->size;
        extraBlock->linkAfter(freeBlock);

        // insert the free block into the system
        if (freeBlock->size >= m_minFreeBlockSize)
            m_freeBlocks.insert(FreeBlock(freeBlock));

        // the big block becomes the free block
        freeBlock = extraBlock;
    }

    // extract the free block at the end of the allocation
    if (splitEnd < freeBlock->offset + freeBlock->size)
    {
        // leave the current free block where it is just shrink its size
        auto originalSize = freeBlock->size;
        freeBlock->size = splitEnd - splitStart;

        // move rest of the memory to the new block that starts at the exact position of the split
        auto extraBlock  = m_blockPool.create();
        extraBlock->allocated = 0;
        extraBlock->offset = splitEnd;
        extraBlock->size = originalSize - freeBlock->size;
        extraBlock->linkAfter(freeBlock);

        // insert the free block into the system
        if (extraBlock->size >= m_minFreeBlockSize)
            m_freeBlocks.insert(FreeBlock(extraBlock));
    }

    // the free block should be EXACT size of the allocation
    // allocate it by changing the flag
    auto requiredSize = (splitEnd - splitStart);
    ASSERT(freeBlock->size == requiredSize);
    freeBlock->allocated = 1;
    return freeBlock;
}

BlockAllocationResult BlockPool::allocateBlock(uint32_t size, uint32_t alignment, MemoryBlock& outBlock)
{
    // empty block cannot be allocated
    if (!size)
        return BlockAllocationResult::InvalidBlock;

    // we don't have enough blocks
    if (m_numAllocatedBlocks == m_totalBlocks)
        return BlockAllocationResult::OutOfBlocks;

    // find the free block to accommodate the request
    auto alignedSize = base::Align(size, alignment);
    auto freeBlockIndex = firstBestFreeBlock(alignedSize, alignment);
    if (freeBlockIndex == INDEX_NONE)
    {
        auto totalFreeSize = m_totalSize - m_numAllocatedBytes;
        if (size <= totalFreeSize)
            return BlockAllocationResult::FragmentationError;
        else
            return BlockAllocationResult::OutOfMemory;
    }

    // extract the free block, we will reinsert the remaining free memory if we find anything
    auto freeBlock  = m_freeBlocks[freeBlockIndex].m_block;
    m_freeBlocks.erase(freeBlockIndex);

    // get the offset where the allocation happens
    auto splitStart = base::Align(freeBlock->offset, range_cast<uint64_t>(alignment));
    auto splitEnd = splitStart + alignedSize;

    // extract the allocation block, this splits the free block into the allocated and free parts
    auto allocatedBlock  = extractUsedBlock(freeBlock, splitStart, splitEnd);
    ASSERT(allocatedBlock != nullptr);

    // update stats
    m_numAllocatedBlocks += 1;
    m_numAllocatedBytes += alignedSize;
    m_maxAllocatedBytes = std::max(m_maxAllocatedBytes, m_numAllocatedBytes);
    m_maxAllocatedBlocks = std::max(m_maxAllocatedBlocks, m_numAllocatedBlocks);

    // return the block
	ASSERT(allocatedBlock->allocated == 1);
    outBlock = allocatedBlock;
    validate();
    return BlockAllocationResult::OK;
}

void BlockPool::freeBlock(MemoryBlock block)
{
    ASSERT(block != nullptr);
    ASSERT(m_numAllocatedBlocks > 0);

    auto nativeBlock  = (Block*)block;
    ASSERT(nativeBlock->allocated);

    // update stats
    ASSERT(nativeBlock->size <= m_numAllocatedBytes);
    m_numAllocatedBytes -= nativeBlock->size;
    m_numAllocatedBlocks -= 1;

    // mark block as free
    nativeBlock->allocated = 0;

    // merge with the next free block
    {
        auto nextFree  = nativeBlock->next;
        if (nextFree)
        {
            ASSERT(nextFree->offset == nativeBlock->offset + nativeBlock->size);
            if (!nextFree->allocated)
            {
                nativeBlock->size += nextFree->size;
                m_freeBlocks.remove(FreeBlock(nextFree));
                nextFree->unlink();
                m_blockPool.free(nextFree);
            }
        }
    }

    // merge with the previous block
    {
        auto prevFree  = nativeBlock->prev;
        if (prevFree)
        {
            ASSERT(prevFree->offset + prevFree->size == nativeBlock->offset);
            if (!prevFree->allocated)
            {
                nativeBlock->offset = prevFree->offset;
                nativeBlock->size += prevFree->size;
                m_freeBlocks.remove(FreeBlock(prevFree));
                prevFree->unlink();
                if (prevFree == m_blockList)
                    m_blockList = nativeBlock;
                m_blockPool.free(prevFree);
            }
        }
    }

    // insert the free entry (merged) into the free list
    if (nativeBlock->size >= m_minFreeBlockSize)
        m_freeBlocks.insert(FreeBlock(nativeBlock));
    validate();
}

void BlockPool::requestBlocksToMove(uint64_t maxBytesToMove, uint32_t maxBlocksToMove, base::Array<BlockToDefrag>& outBlocks)
{
    // todo
}

void BlockPool::signalBlockMoved(MemoryBlock id)
{
    // todo
}

END_BOOMER_NAMESPACE(base)

