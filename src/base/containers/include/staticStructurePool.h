/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

///--

// static (non resizable) table with IDs to elements - typeless implementation
struct BASE_CONTAINERS_API StaticStructurePoolBase : public NoCopy
{
public:
    StaticStructurePoolBase(PoolTag pool, uint32_t elemSize, uint32_t elemAlign);
    ~StaticStructurePoolBase();

    // maximum capacity
    INLINE uint32_t capacity() const { return m_maxAllocated; }

    // current number of allocated elements
    INLINE uint32_t occupancy() const { return m_numAllocated; }

    // get bitmap
    INLINE const uint64_t* elementBitMapPtr() const { return m_elementBitMap; }

    // get bitmap end (for iterations)
    INLINE const uint64_t* elementBitMapEndPtr() const { return m_elementBitMapEnd; }

    // is the pool full ?
    INLINE bool full() const { return m_numAllocated == m_maxAllocated; }

    // raw data
    INLINE void* data() { return m_elements; }

    // raw data (readonly)
    INLINE const void* data() const { return m_elements; }

    //--

    // release all elements from the pool, allocated elements are destroyed
    void clear();

    // resize pool to new size, allocated elements are moved but the IDs are NOT CANGED (no defrag)
    // NOTE: it's not possible to make pool smaller without clearing it first
    void resize(uint32_t capacity);

    // allocate entry from pool, asserts if pool was full
    uint32_t allocateIndex();

    // free entry
    void freeIndex(uint32_t index);

    //--

    // is given entry allocated ?
    ALWAYS_INLINE bool checkBit(uint32_t index) const
    {
        const auto bitMask = 1ULL << (index & 63);
        return 0 != (m_elementBitMap[index / 64] & bitMask);
    }

private:
    void* m_elements = nullptr;
    uint32_t m_numAllocated = 0;
    uint32_t m_maxAllocated = 0;

    uint64_t* m_elementBitMap = nullptr;
    uint64_t* m_elementBitMapEnd = nullptr;
    uint32_t m_freeBucketIndex = 0;

    uint32_t m_elemSize = 0;
    uint32_t m_elemAlign = 0;

    PoolTag m_pool;

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
    
///--

// static (non resizable) table with IDs to elements
template< typename T >
struct StaticStructurePool : protected StaticStructurePoolBase
{
public:
    INLINE StaticStructurePool(PoolTag pool);
    INLINE ~StaticStructurePool();

    //--

    // maximum capacity
    INLINE uint32_t capacity() const { return StaticStructurePoolBase::capacity(); }

    // current number of allocated elements
    INLINE uint32_t occupancy() const { return StaticStructurePoolBase::occupancy(); }

    // is the pool full ?
    INLINE bool full() const { return StaticStructurePoolBase::full(); }

    // element data
    INLINE T* typedData() { return (T*)StaticStructurePoolBase::data(); }

    // element data
    INLINE const T* typedData() const { return (const T*)StaticStructurePoolBase::data(); }

    //--

    // release all elements from the pool, allocated elements are destroyed
    void clear();

    // resize pool to new size, allocated elements are moved but the IDs are NOT CANGED (no defrag)
    // NOTE: it's not possible to make pool smaller without clearing it first
    void resize(uint32_t capacity);

    // emplace element data, asserts if pool is full
    template<typename ...Args>
    T& emplaceWithIndex(uint32_t* outIndex, Args&&... args);

    // emplace element data, asserts if pool is full
    template<typename ...Args>
    uint32_t emplace(Args&&... args);

    // free entry
    void free(uint32_t index);

    //--

    // visit all allocated elements
    bool enumerate(const std::function<bool(T& elem, uint32_t index)>& enumFunc);

    // visit all allocated elements (read only version)
    bool enumerate(const std::function<bool(const T& elem, uint32_t index)>& enumFunc) const;

    //--
};

//--

END_BOOMER_NAMESPACE(base)

#include "staticStructurePool.inl"