/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//--

template< typename T >
INLINE StaticStructurePool<T>::StaticStructurePool(PoolTag pool)
    : StaticStructurePoolBase(pool, sizeof(T), alignof(T))
{}

template< typename T >
INLINE StaticStructurePool<T>::~StaticStructurePool()
{
    clear();
}

template< typename T >
INLINE void StaticStructurePool<T>::clear()
{
    StaticStructurePoolBase::clear();
}

template< typename T >
INLINE void StaticStructurePool<T>::resize(uint32_t capacity)
{
    StaticStructurePoolBase::resize(capacity);
}

template< typename T >
template<typename ...Args>
INLINE T& StaticStructurePool<T>::emplaceWithIndex(uint32_t* outIndex, Args&&... args)
{
    auto index = StaticStructurePoolBase::allocateIndex();
    new (typedData() + index) T(std::forward<Args>(args)...);
    if (outIndex)
        *outIndex = index;
    return typedData()[index];
}

template< typename T >
template<typename ...Args>
INLINE uint32_t StaticStructurePool<T>::emplace(Args&&... args)
{
    auto index = StaticStructurePoolBase::allocateIndex();
    new (typedData() + index) T(std::forward<Args>(args)...);
    return index;
}

template< typename T >
INLINE void StaticStructurePool<T>::free(uint32_t index)
{
    ASSERT_EX(checkBit(index), "Element is not allocated");

    auto* elemPtr = typedData() + index;
    elemPtr->~T();

    StaticStructurePoolBase::freeIndex(index);
}

template< typename T >
bool StaticStructurePool<T>::enumerate(const std::function<bool(T & elem, uint32_t index)>& enumFunc)
{
    uint32_t index = 0;
    uint32_t numVisited = 0;
    const auto* ptr = elementBitMapPtr();
    while (ptr < elementBitMapEndPtr())
    {
        auto mask = *ptr;
        while (mask)
        {
            uint32_t localIndex = __builtin_ctzll(mask) + index;
            DEBUG_CHECK(checkBit(localIndex));
            if (enumFunc(typedData()[localIndex], localIndex))
                return true;
            mask ^= mask & -mask;
            numVisited += 1;
        }

        ptr++;
        index += 64;
    }

    DEBUG_CHECK_EX(numVisited == occupancy(), "Strange count of visited items");
    return false;
}

template< typename T >
bool StaticStructurePool<T>::enumerate(const std::function<bool(const T & elem, uint32_t index)>& enumFunc) const
{
    uint32_t index = 0;
    uint32_t numVisited = 0;
    const auto* ptr = elementBitMapPtr();
    while (ptr < elementBitMapEndPtr())
    {
        auto mask = *ptr;
        while (mask)
        {
            uint32_t localIndex = __builtin_ctzll(mask) + index;
            DEBUG_CHECK(checkBit(localIndex));
            if (enumFunc(typedData()[localIndex], localIndex))
                return true;
            mask ^= mask & -mask;
            numVisited += 1;
        }

        ptr++;
        index += 64;
    }

    DEBUG_CHECK_EX(numVisited == occupancy(), "Strange count of visited items");
    return false;
}

END_BOOMER_NAMESPACE(base)
