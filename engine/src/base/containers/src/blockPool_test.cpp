/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: memory #]
*/

#include "build.h"
#include "blockPool.h"

#include "base/test/include/gtest/gtest.h"

using namespace base;

DECLARE_TEST_FILE(BlockPool);

TEST(BlockPool, EmptyTest)
{
    BlockPool allocator;
    BlockPoolStats stats;
    allocator.stats(stats);
    EXPECT_EQ(0, stats.totalPoolSize);
    EXPECT_EQ(0, stats.totalBlockCount);
    EXPECT_EQ(0, stats.currentAllocatedBytes);
    EXPECT_EQ(0, stats.maxAllocatedBytes);
    EXPECT_EQ(0, stats.currentAllocatedBlocks);
    EXPECT_EQ(0, stats.maxAllocatedBlocks);
    EXPECT_EQ(0, stats.defragPendingBytes);
    EXPECT_EQ(0, stats.defragPendingBlocks);
    EXPECT_EQ(0, stats.largestFreeBlockBytes);
    EXPECT_EQ(0, stats.freeBlockCount);
}

TEST(BlockPool, InitTest)
{
    BlockPool allocator;
    allocator.setup(1024, 10, 1);

    BlockPoolStats stats;
    allocator.stats(stats);
    EXPECT_EQ(1024, stats.totalPoolSize);
    EXPECT_EQ(10, stats.totalBlockCount);
    EXPECT_EQ(0, stats.currentAllocatedBytes);
    EXPECT_EQ(0, stats.maxAllocatedBytes);
    EXPECT_EQ(0, stats.currentAllocatedBlocks);
    EXPECT_EQ(0, stats.maxAllocatedBlocks);
    EXPECT_EQ(0, stats.defragPendingBytes);
    EXPECT_EQ(0, stats.defragPendingBlocks);
    EXPECT_EQ(1024, stats.largestFreeBlockBytes);
    EXPECT_EQ(1, stats.freeBlockCount);
}

TEST(BlockPool, SingleAllocTest)
{
    BlockPool allocator;
    allocator.setup(1024, 10, 1);

    MemoryBlock block = INVALID_MEMORY_BLOCK;
    auto ret  = allocator.allocateBlock(512, 1, block);
    ASSERT_EQ(BlockAllocationResult::OK, ret);
    ASSERT_TRUE(block != INVALID_MEMORY_BLOCK);

    BlockPoolStats stats;
    allocator.stats(stats);
    EXPECT_EQ(1024, stats.totalPoolSize);
    EXPECT_EQ(10, stats.totalBlockCount);
    EXPECT_EQ(512, stats.currentAllocatedBytes);
    EXPECT_EQ(512, stats.maxAllocatedBytes);
    EXPECT_EQ(1, stats.currentAllocatedBlocks);
    EXPECT_EQ(1, stats.maxAllocatedBlocks);
    EXPECT_EQ(512, stats.largestFreeBlockBytes);
    EXPECT_EQ(1, stats.freeBlockCount);

    allocator.freeBlock(block);
}

TEST(BlockPool, SingleAllocAndFreeTest)
{
    BlockPool allocator;
    allocator.setup(1024, 10, 1);

    MemoryBlock block = INVALID_MEMORY_BLOCK;
    allocator.allocateBlock(512, 1, block);
    ASSERT_TRUE(block != INVALID_MEMORY_BLOCK);
    allocator.freeBlock(block);

    BlockPoolStats stats;
    allocator.stats(stats);
    EXPECT_EQ(1024, stats.totalPoolSize);
    EXPECT_EQ(10, stats.totalBlockCount);
    EXPECT_EQ(0, stats.currentAllocatedBytes);
    EXPECT_EQ(512, stats.maxAllocatedBytes);
    EXPECT_EQ(0, stats.currentAllocatedBlocks);
    EXPECT_EQ(1, stats.maxAllocatedBlocks);
    EXPECT_EQ(1024, stats.largestFreeBlockBytes);
    EXPECT_EQ(1, stats.freeBlockCount);
}

TEST(BlockPool, OutOfMemoryTest)
{
    BlockPool allocator;
    allocator.setup(1024, 10, 1);

    MemoryBlock block1 = INVALID_MEMORY_BLOCK;
    allocator.allocateBlock(512, 1, block1);

    MemoryBlock block2 = INVALID_MEMORY_BLOCK;
    auto ret  = allocator.allocateBlock(1024, 1, block2);
    EXPECT_EQ(BlockAllocationResult::OutOfMemory, ret);

    allocator.freeBlock(block1);
}

TEST(BlockPool, OutOfBlocksTest)
{
    BlockPool allocator;
    allocator.setup(1024, 1, 1);

    MemoryBlock block1 = INVALID_MEMORY_BLOCK;
    allocator.allocateBlock(128, 1, block1);

    MemoryBlock block2 = INVALID_MEMORY_BLOCK;
    auto ret  = allocator.allocateBlock(128, 1, block2);
    EXPECT_EQ(BlockAllocationResult::OutOfBlocks, ret);

    allocator.freeBlock(block1);
}

TEST(BlockPool, MergeFreeTest)
{
    BlockPool allocator;
    allocator.setup(1024, 10, 1);

    MemoryBlock block1 = INVALID_MEMORY_BLOCK;
    MemoryBlock block2 = INVALID_MEMORY_BLOCK;
    MemoryBlock block3 = INVALID_MEMORY_BLOCK;
    EXPECT_EQ(BlockAllocationResult::OK, allocator.allocateBlock(256, 1, block1));
    EXPECT_EQ(BlockAllocationResult::OK, allocator.allocateBlock(512, 1, block2));
    EXPECT_EQ(BlockAllocationResult::OK, allocator.allocateBlock(256, 1, block3));

    BlockPoolStats stats;
    allocator.stats(stats);
    EXPECT_EQ(0, stats.largestFreeBlockBytes);
    EXPECT_EQ(0, stats.freeBlockCount);

    // free side blocks
    allocator.freeBlock(block1);
    allocator.freeBlock(block3);

    allocator.stats(stats);
    EXPECT_EQ(256, stats.largestFreeBlockBytes);
    EXPECT_EQ(2, stats.freeBlockCount);

    // free center block
    allocator.freeBlock(block2);

    allocator.stats(stats);
    EXPECT_EQ(1024, stats.largestFreeBlockBytes);
    EXPECT_EQ(1, stats.freeBlockCount);
}

TEST(BlockPool, AlignBlockSplitTest)
{
    BlockPool allocator;
    allocator.setup(1024, 10, 1);

    MemoryBlock block1 = INVALID_MEMORY_BLOCK;
    MemoryBlock block2 = INVALID_MEMORY_BLOCK;
    MemoryBlock block3 = INVALID_MEMORY_BLOCK;
    EXPECT_EQ(BlockAllocationResult::OK, allocator.allocateBlock(20, 1, block1));
    EXPECT_EQ(BlockAllocationResult::OK, allocator.allocateBlock(512, 512, block2));

    BlockPoolStats stats;
    allocator.stats(stats);
    EXPECT_EQ(512 - 20, stats.largestFreeBlockBytes);
    EXPECT_EQ(1, stats.freeBlockCount);

    // free side blocks
    allocator.freeBlock(block1);
    allocator.freeBlock(block2);
}

TEST(BlockPool, StressTestNormalWithoutDefrag)
{
    BlockPool allocator;
    allocator.setup(1024 * 1024, 1024, 64);

    base::Array<MemoryBlock> allocatedBlocks;
    uint32_t numFailedAllocations = 0;

    // allocate initial block count
    static auto count  = 1024;
    for (auto i  = 0; i < count; ++i)
    {
        auto size  = 1 + rand() % 1024;
        auto alignment  = 1 + rand() % 64;

        MemoryBlock block;
        auto ret  = allocator.allocateBlock(size, alignment, block);
        if (ret == BlockAllocationResult::OK)
            allocatedBlocks.pushBack(block);
        else
            numFailedAllocations += 1;
    }

    // do the stress test
    for (uint32_t i = 0; i < 10; ++i)
    {
        BlockPoolStats stats;
        allocator.stats(stats);
        TRACE_INFO("Round[{}]: {} blocks allocated, {} bytes, {} free blocks, {} largest free block, {} wasted, {} failed allocations",
                   i,
                   stats.currentAllocatedBlocks, stats.currentAllocatedBytes,
                   stats.freeBlockCount, stats.largestFreeBlockBytes,
                   stats.wastedBytes,
                   numFailedAllocations);

        // free 25% of random blocks
        for (auto i  = 0; i < count / 4; ++i)
        {
            auto index  = rand() % allocatedBlocks.size();
            auto block  = allocatedBlocks[index];
            allocator.freeBlock(block);

            std::swap(allocatedBlocks[index], allocatedBlocks.back());
            allocatedBlocks.popBack();
        }

        // allocate 25% of new blocks
        for (auto i  = 0; i < count / 4; ++i)
        {
            auto size  = 1 + rand() % 1024;
            auto alignment  = 1 + rand() % 64;

            MemoryBlock block;
            auto ret  = allocator.allocateBlock(size, alignment, block);
            if (ret == BlockAllocationResult::OK)
                allocatedBlocks.pushBack(block);
            else
                numFailedAllocations += 1;
        }
    }

    // free all blocks
    for (auto block  : allocatedBlocks)
        allocator.freeBlock(block);

    // we must be back to one big block
    BlockPoolStats stats;
    allocator.stats(stats);
    EXPECT_EQ(1024*1024, stats.largestFreeBlockBytes);
    EXPECT_EQ(1, stats.freeBlockCount);
}

TEST(BlockPool, StressTestHardWithoutDefrag)
{
    BlockPool allocator;
    allocator.setup(1024 * 1024, 2048, 64);

    base::Array<MemoryBlock> allocatedBlocks;
    uint32_t numFailedAllocations = 0;

    // allocate initial block count
    static auto count  = 2048;
    for (auto i  = 0; i < count; ++i)
    {
        auto size  = 1 + rand() % 1024;
        auto alignment  = 1 + rand() % 64;

        MemoryBlock block;
        auto ret  = allocator.allocateBlock(size, alignment, block);
        if (ret == BlockAllocationResult::OK)
            allocatedBlocks.pushBack(block);
        else
            numFailedAllocations += 1;
    }

    // do the stress test
    for (uint32_t i = 0; i < 10; ++i)
    {
        BlockPoolStats stats;
        allocator.stats(stats);
        TRACE_INFO("Round[{}]: {} blocks allocated, {} bytes, {} free blocks, {} largest free block, {} wasted, {} failed allocations",
                   i,
                   stats.currentAllocatedBlocks, stats.currentAllocatedBytes,
                   stats.freeBlockCount, stats.largestFreeBlockBytes,
                   stats.wastedBytes,
                   numFailedAllocations);

        // free 25% of random blocks
        for (auto i  = 0; i < count / 4; ++i)
        {
            auto index  = rand() % allocatedBlocks.size();
            auto block  = allocatedBlocks[index];
            allocator.freeBlock(block);

            std::swap(allocatedBlocks[index], allocatedBlocks.back());
            allocatedBlocks.popBack();
        }

        // allocate 25% of new blocks
        for (auto i = 0; i < count / 4; ++i)
        {
            auto size = 1 + rand() % 1024;
            auto alignment = 1 + rand() % 64;

            MemoryBlock block;
            auto ret = allocator.allocateBlock(size, alignment, block);
            if (ret == BlockAllocationResult::OK)
                allocatedBlocks.pushBack(block);
            else
                numFailedAllocations += 1;
        }
    }

    // free all blocks
    for (auto block : allocatedBlocks)
        allocator.freeBlock(block);

    // we must be back to one big block
    BlockPoolStats stats;
    allocator.stats(stats);
    EXPECT_EQ(1024 * 1024, stats.largestFreeBlockBytes);
    EXPECT_EQ(1, stats.freeBlockCount);
}