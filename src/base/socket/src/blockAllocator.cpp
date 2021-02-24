/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: raw #]
***/

#include "build.h"
#include "block.h"
#include "blockAllocator.h"

BEGIN_BOOMER_NAMESPACE(base::socket)

//--

BlockAllocator::BlockAllocator()
    : m_numBlocks(0)
    , m_numBytes(0)
    , m_maxBlocks(0)
    , m_maxBytes(0)
{}

BlockAllocator::~BlockAllocator()
{
    ASSERT_EX(m_numBlocks.load() == 0, "Blocks from this allocator are still allocated");
}

Block* BlockAllocator::alloc(uint32_t size)
{
    // create memory block
    // TODO: reuse!
    auto totalMemSize = sizeof(Block) + size;
    auto ret = mem::GlobalPool<POOL_NET, Block>::Alloc(totalMemSize, 1);
    ret->m_ptr = (uint8_t*)ret + sizeof(Block);
    ret->m_totalSize = totalMemSize;
    ret->m_currentSize = size;
    ret->m_allocator = this;

    // stats
    auto numBlocks = ++m_numBlocks;
    auto numBytes = m_numBytes += size;
    AtomicMax(m_maxBlocks, numBlocks);
    AtomicMax(m_maxBytes, numBytes + size);

    return ret;
}

void BlockAllocator::releaseBlock(Block* block)
{
    ASSERT(m_numBlocks.load() > 0);
    m_numBytes -= block->m_totalSize;
    m_numBlocks--;
    mem::GlobalPool<POOL_NET, Block>::Free(block);
}

Block* BlockAllocator::build(std::initializer_list<BlockPart> blocks)
{
    uint32_t totalSize = 0;
    for (auto &b : blocks)
        totalSize += b.size;

    auto ret  = alloc(totalSize);
    if (ret)
    {
        auto writePtr  = ret->m_ptr;
        for (auto &b : blocks)
        {
            memcpy((void*)writePtr, b.dataPtr, b.size);
            writePtr += b.size;
        }
    }

    return ret;
}

Block* BlockAllocator::build(const Array<BlockPart>& blocks)
{
    uint32_t totalSize = 0;
    for (auto &b : blocks)
        totalSize += b.size;

    auto ret  = alloc(totalSize);
    if (ret)
    {
        auto writePtr  = ret->m_ptr;
        for (auto &b : blocks)
        {
            memcpy((void*)writePtr, b.dataPtr, b.size);
            writePtr += b.size;
        }
    }

    return ret;
}

//--

END_BOOMER_NAMESPACE(base::socket)
