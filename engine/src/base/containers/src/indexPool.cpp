/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#include "build.h"
#include "indexPool.h"

namespace base
{

    //--

    IndexPool::IndexPool(mem::PoolID pool)
        : m_pool(pool)
    {}

    IndexPool::~IndexPool()
    {
        clear();
    }

    void IndexPool::clear()
    {
        MemFree(m_elementBitMap);
        m_elementBitMap = nullptr;
        m_elementBitMapEnd = nullptr;
        m_freeBucketIndex = 0;
        m_numAllocated = 0;
        m_maxAllocated = 0;
    }

    void IndexPool::resize(uint32_t capacity)
    {
        auto alignedCapacity = Align<uint32_t>(capacity, 64);
        if (alignedCapacity > m_maxAllocated)
        {
            m_elementBitMap = (uint64_t*)MemRealloc(m_pool, m_elementBitMap, alignedCapacity / 8, 8);
            memset(m_elementBitMap + (m_maxAllocated / 8), 0, (alignedCapacity - m_maxAllocated) / 8);
            m_elementBitMapEnd = m_elementBitMap + (alignedCapacity / 8);

            m_maxAllocated = alignedCapacity;
        }
    }

    bool IndexPool::allocate(uint32_t count, uint32_t& outFirst)
    {
        if (!count)
        {
            outFirst = 0;
            return true;
        }

        if (m_numAllocated + count > m_maxAllocated)
            return false;

        // TODO !
        return false;

    }

    void IndexPool::free(uint32_t index, uint32_t count)
    {

    }

    //--

} // bas