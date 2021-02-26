/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "array.h"
#include "arrayIterator.h"
#include "hash.inl"

BEGIN_BOOMER_NAMESPACE()

//---

template<typename K>
INLINE HashSet<K>::HashSet(const HashSet<K>& other)
    : m_keys(other.m_keys)
{
    HashBuckets::Build(m_buckets, m_keys.typedData(), m_keys.size(), m_keys.capacity());
}

template<typename K>
INLINE HashSet<K>::HashSet(HashSet<K>&& other)
    : m_keys(std::move(other.m_keys))
{
    m_buckets = other.m_buckets;
    other.m_buckets = nullptr;
}

template<typename K>
INLINE HashSet<K>& HashSet<K>::operator=(const HashSet<K>& other)
{
    if (this != &other)
    {
        m_keys = other.m_keys;
        HashBuckets::Build(m_buckets, m_keys.typedData(), m_keys.size(), m_keys.capacity());
    }

    return *this;
}

template<typename K>
INLINE HashSet<K>& HashSet<K>::operator=(HashSet<K>&& other)
{
    if (this != &other)
    {
        m_keys = std::move(other.m_keys);

        HashBuckets::Clear(m_buckets);
        m_buckets = other.m_buckets;
        other.m_buckets = nullptr;
    }

    return *this;
}

template<typename K>
INLINE HashSet<K>::~HashSet()
{
    HashBuckets::Clear(m_buckets);
}

//---

template<typename K>
INLINE void HashSet<K>::clear()
{
    m_keys.clear();
    HashBuckets::Clear(m_buckets);
}

template<typename K>
INLINE void HashSet<K>::reset()
{
    m_keys.reset();
    HashBuckets::Reset(m_buckets);
}

template<typename K>
INLINE bool HashSet<K>::empty() const
{
    return m_keys.empty();
}

template<typename K>
INLINE uint32_t HashSet<K>::size() const
{
    return m_keys.size();
}

template<typename K>
void HashSet<K>::reserve(uint32_t size)
{
    if (size > m_keys.capacity())
    {
        m_keys.reserve(size);

        if (HashBuckets::CheckCapacity(m_buckets, m_keys.capacity()))
            HashBuckets::Build(m_buckets, m_keys.typedData(), m_keys.size(), m_keys.capacity());
    }
}

template<typename K>
bool HashSet<K>::insert(const K& key)
{
    uint32_t index = 0;
    if (HashBuckets::Find(m_buckets, m_keys.typedData(), m_keys.size(), key, index))
        return false;

    m_keys.emplaceBack(key);

    if (HashBuckets::CheckCapacity(m_buckets, m_keys.size())) // check capacity with actual number of elements, not the array capacity
        HashBuckets::Insert(m_buckets, key, m_keys.lastValidIndex());
    else
        HashBuckets::Build(m_buckets, m_keys.typedData(), m_keys.size(), m_keys.capacity()); // when rehashing adapt for current arrays capacity

    return true;
}

template<typename K>
template<typename FK >
bool HashSet<K>::remove(const FK& key)
{
    uint32_t index = 0;
    if (HashBuckets::Remove(m_buckets, m_keys.typedData(), m_keys.size(), key, index))
    {
        m_keys.eraseUnordered(index);
        return true;
    }

    return false;
}

template<typename K>
template< typename FK >
bool HashSet<K>::contains(const FK& key) const
{
    uint32_t index = 0;
    return HashBuckets::Find(m_buckets, m_keys.typedData(), m_keys.size(), key, index);
}

//---

END_BOOMER_NAMESPACE()
