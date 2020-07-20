/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "base/memory/include/poolID.h"

namespace base
{
    ///--

    // static (non resizable) ID allocator that supports allocating range of entries
    struct BASE_CONTAINERS_API IndexPool : public NoCopy
    {
    public:
        IndexPool(mem::PoolID pool);
        ~IndexPool();

        // maximum capacity
        INLINE uint32_t capacity() const { return m_maxAllocated; }

        // current number of allocated elements
        INLINE uint32_t occupancy() const { return m_numAllocated; }

        // raw data - bits
        INLINE uint64_t* data() { return m_elementBitMap; }

        // raw data (readonly)
        INLINE const uint64_t* data() const { return m_elementBitMap; }

        //--

        // release all elements from the pool, allocated elements are destroyed
        void clear();

        // resize pool to new size, allocated elements are moved but the IDs are NOT CANGED (no defrag)
        // NOTE: it's not possible to make pool smaller without clearing it first
        void resize(uint32_t capacity);

        // allocate single bit entry, fast
        bool allocate(uint32_t count, uint32_t& outFirst);

        // free bits
        void free(uint32_t index, uint32_t count);

        //--

        // is given entry allocated ?
        ALWAYS_INLINE bool checkBit(uint32_t index) const
        {
            const auto bitMask = 1ULL << (index & 63);
            return 0 != (m_elementBitMap[index / 64] & bitMask);
        }

    private:
        uint32_t m_numAllocated = 0;
        uint32_t m_maxAllocated = 0;

        uint64_t* m_elementBitMap = nullptr;
        uint64_t* m_elementBitMapEnd = nullptr;
        uint32_t m_freeBucketIndex = 0;

        mem::PoolID m_pool;

        ALWAYS_INLINE bool setBit(uint32_t index)
        {
            const auto bitMask = 1ULL << (index & 63);
            auto mask = m_elementBitMap[index / 64];
            m_elementBitMap[index / 64] |= bitMask;
            return 0 != (mask & bitMask);
        }

        ALWAYS_INLINE bool clearBit(uint32_t index)
        {
            const auto bitMask = 1ULL << (index & 63);
            auto mask = m_elementBitMap[index / 64];
            m_elementBitMap[index / 64] &= ~bitMask;
            return 0 != (mask & bitMask);
        }
    };
    
    //--

} // base
