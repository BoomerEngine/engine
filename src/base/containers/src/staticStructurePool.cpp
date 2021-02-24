/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#include "build.h"
#include "staticStructurePool.h"
#include "bitUtils.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

StaticStructurePoolBase::StaticStructurePoolBase(PoolTag pool, uint32_t elemSize, uint32_t elemAlign)
    : m_pool(pool)
    , m_elemSize(elemSize)
    , m_elemAlign(elemAlign)
{
}

StaticStructurePoolBase::~StaticStructurePoolBase()
{
    clear();
}

void StaticStructurePoolBase::clear()
{
    if (m_elements)
    {
        mem::FreeBlock(m_elements);
        m_elements = nullptr;
        m_maxAllocated = 0;
        m_numAllocated = 0;

        mem::FreeBlock(m_elementBitMap);
        m_elementBitMap = nullptr;
        m_elementBitMapEnd = nullptr;
        m_freeBucketIndex = 0;
    }
}

void StaticStructurePoolBase::resize(uint32_t capacity)
{
    auto alignedCapacity = Align<uint32_t>(capacity, 64);
    if (alignedCapacity > m_maxAllocated)
    {
        m_elements = mem::ResizeBlock(m_pool, m_elements, m_elemSize * alignedCapacity, m_elemAlign, "StructurePool");
#ifndef BUILD_RELEASE
        memset((uint8_t*)m_elements + (m_elemSize*m_maxAllocated), 0xCC, m_elemSize * (alignedCapacity - m_maxAllocated));
#endif

        m_elementBitMap = (uint64_t*)mem::ResizeBlock(m_pool, m_elementBitMap, alignedCapacity / 8, 8, "StructurePool");
        memset(m_elementBitMap + (m_maxAllocated / 8), 0, (alignedCapacity - m_maxAllocated) / 8);
        m_elementBitMapEnd = m_elementBitMap + (alignedCapacity / 64);

        m_maxAllocated = alignedCapacity;
    }
}

uint32_t StaticStructurePoolBase::allocateIndex()
{
    ASSERT_EX(!full(), "Trying to allocate from full structure pool");

    auto* ptr = m_elementBitMap + m_freeBucketIndex;
    while (ptr < m_elementBitMapEnd)
    {
        const auto freeMask = ~*ptr;
        if (freeMask)
        {
            uint32_t freeIndex = __builtin_ctzll(freeMask) + (m_freeBucketIndex * 64);
            if (!setBit(freeIndex))
            {
                m_numAllocated += 1;
                return freeIndex;
            }
        }

        m_freeBucketIndex += 1;
        ++ptr;
    }

    ASSERT(!"No free bit in the bit mask");
    return INDEX_MAX;
}

void StaticStructurePoolBase::freeIndex(uint32_t index)
{
    ASSERT_EX(index < m_maxAllocated, "Index out of range");
    ASSERT_EX(checkBit(index), "Entry is not allocated");
    ASSERT_EX(m_numAllocated > 0, "Nothing is allocated");

    if (clearBit(index))
    {
        auto* elemPtr = (uint8_t*)m_elements + (index * m_elemSize);
#ifndef BUILD_RELEASE
        memset(elemPtr, 0xFA, m_elemSize);
#endif

        auto bucketIndex = index / 64;
        if (bucketIndex < m_freeBucketIndex)
            m_freeBucketIndex = bucketIndex;
            
        m_numAllocated -= 1;
    }
}
    
//--

END_BOOMER_NAMESPACE(base)
