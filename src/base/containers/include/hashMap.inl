/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "hash.inl"

BEGIN_BOOMER_NAMESPACE(base)

//--

template< class K, class V >
ALWAYS_INLINE HashMap<K, V>::HashMap(const HashMap<K, V>& other)
    : m_keys(other.m_keys)
    , m_values(other.m_values)
{
    HashBuckets::Build(m_buckets, m_keys.typedData(), m_keys.size(), m_keys.capacity());
}

template< class K, class V >
ALWAYS_INLINE HashMap<K, V>::HashMap(HashMap<K, V>&& other)
    : m_keys(std::move(other.m_keys))
    , m_values(std::move(other.m_values))
{
    m_buckets = other.m_buckets;
    other.m_buckets = nullptr;
}

template< class K, class V >
ALWAYS_INLINE HashMap<K, V>& HashMap<K, V>::operator=(const HashMap<K, V>& other)
{
    if (this != &other)
    {
        m_keys = other.m_keys;
        m_values = other.m_values;
        HashBuckets::Build(m_buckets, m_keys.typedData(), m_keys.size(), m_keys.capacity());
    }

    return *this;
}

template< class K, class V >
ALWAYS_INLINE HashMap<K, V>& HashMap<K, V>::operator=(HashMap<K, V>&& other)
{
    if (this != &other)
    {
        m_keys = std::move(other.m_keys);
        m_values = std::move(other.m_values);

        HashBuckets::Clear(m_buckets);
        m_buckets = other.m_buckets;
        other.m_buckets = nullptr;
    }

    return *this;
}

template< class K, class V >
ALWAYS_INLINE HashMap<K,V>::~HashMap()
{
    HashBuckets::Clear(m_buckets);
}

//--

template< class K, class V >
INLINE void HashMap<K,V>::clear()
{
    m_keys.clear();
    m_values.clear();
    HashBuckets::Clear(m_buckets);
}

template< class K, class V >
INLINE void HashMap<K,V>::clearPtr()
{
    m_values.clearPtr();
    clear();
}

template< class K, class V >
INLINE bool HashMap<K,V>::empty() const
{
    ASSERT(m_values.empty() == m_keys.empty());
    return m_values.empty();
}

template< class K, class V >
INLINE uint32_t HashMap<K,V>::size() const
{
    ASSERT(m_values.size() == m_keys.size());
    return m_values.size();
}

template< class K, class V >
INLINE void HashMap<K,V>::reserve(uint32_t size)
{
    if (size > m_keys.capacity())
    {
        m_keys.reserve(size);
        m_values.reserve(size);

        if (HashBuckets::CheckCapacity(m_buckets, m_keys.capacity()))
            HashBuckets::Build(m_buckets, m_keys.typedData(), m_keys.size(), m_keys.capacity());
    }
}

template< class K, class V >
INLINE void HashMap<K,V>::reset()
{
    m_keys.reset();
    m_values.reset();
    HashBuckets::Reset(m_buckets);
}

template< class K, class V >
V* HashMap<K,V>::set(const K& key, const V& val)
{
    uint32_t index = 0;
    if (HashBuckets::Find(m_buckets, m_keys.typedData(), m_keys.size(), key, index))
    {
        m_values[index] = val;
        return &m_values[index];
    }

    return add(key, val);
}

template< class K, class V >
template< typename FK >
bool HashMap<K, V>::remove(const FK& key, V* outRemovedValue /*= nullptr*/)
{
    uint32_t index = 0;
    if (HashBuckets::Remove(m_buckets, m_keys.typedData(), m_keys.size(), key, index))
    {
        if (outRemovedValue)
            *outRemovedValue = std::move(m_values.typedData()[index]);

        m_keys.eraseUnordered(index);
        m_values.eraseUnordered(index);
        return true;
    }

    return false;
}

template< class K, class V >
template< typename FK >
V* HashMap<K, V>::find(const FK& key)
{
    uint32_t index = 0;
    if (HashBuckets::Find(m_buckets, m_keys.typedData(), m_keys.size(), key, index))
        return m_values.typedData() + index;

    return nullptr;
}

template< class K, class V >
template< typename FK >
const V* HashMap<K,V>::find(const FK& key) const
{
    uint32_t index = 0;
    if (HashBuckets::Find(m_buckets, m_keys.typedData(), m_keys.size(), key, index))
        return m_values.typedData() + index;

    return nullptr;
}

template< class K, class V >
template< typename FK >
INLINE bool HashMap<K,V>::find(const FK& key, V &output) const
{
    uint32_t index = 0;
    if (HashBuckets::Find(m_buckets, m_keys.typedData(), m_keys.size(), key, index))
    {
        output = m_values.typedData()[index];
        return true;
    }

    return false;
}

template< class K, class V >
template< typename FK >
INLINE const V& HashMap<K,V>::findSafe(const FK& key, const V& defaultValue) const
{
    uint32_t index = 0;
    if (HashBuckets::Find(m_buckets, m_keys.typedData(), m_keys.size(), key, index))
        return m_values.typedData()[index];
    else
        return defaultValue;
}

template< class K, class V >
template< typename FK >
INLINE bool HashMap<K,V>::contains(const FK& key) const
{
    uint32_t index = 0;
    return HashBuckets::Find(m_buckets, m_keys.typedData(), m_keys.size(), key, index);
}

template< class K, class V >
void HashMap<K,V>::append(const HashMap<K,V>& other)
{
    for (auto p : other.pairs())
        set(p.key, p.value);
}

template< class K, class V >
INLINE V* HashMap<K, V>::add(const K& key, const V& val)
{
    m_keys.emplaceBack(key);
    m_values.emplaceBack(val);

    if (HashBuckets::CheckCapacity(m_buckets, m_keys.size())) // check capacity with actual number of elements, not the array capacity
        HashBuckets::Insert(m_buckets, key, m_keys.lastValidIndex());
    else
        HashBuckets::Build(m_buckets, m_keys.typedData(), m_keys.size(), m_keys.capacity()); // when rehashing adapt for current arrays capacity

    return &m_values.back();
}

template< class K, class V >
INLINE V& HashMap<K,V>::operator[](const K& key)
{
    uint32_t index = 0;
    if (HashBuckets::Find(m_buckets, m_keys.typedData(), m_keys.size(), key, index))
        return m_values[index];

    return *add(key, V());
}

template< class K, class V >
INLINE const V& HashMap<K,V>::operator[](const K& key) const
{
    auto ptr = find(key);
    ASSERT_EX(ptr, "Element not found in map even though it was strongly expected");
    return *ptr;
}

//--

template< class K, class V >
ALWAYS_INLINE Array<V>& HashMap<K, V>::values()
{
    return m_values;
}

//! Get the array with values only
template< class K, class V >
ALWAYS_INLINE const Array<V>& HashMap<K, V>::values() const
{
    return m_values;
}

//! Get the array with keys only
template< class K, class V >
ALWAYS_INLINE const Array<K>& HashMap<K, V>::keys() const
{
    return m_keys;
}

template< class K, class V >
ALWAYS_INLINE const PairContainer<K, V> HashMap<K, V>::pairs() const
{
    return PairContainer<K, V>(m_keys.typedData(), m_values.typedData(), size());
}

//! Get table of pairs
template< class K, class V >
ALWAYS_INLINE PairContainer<K, V> HashMap<K, V>::pairs()
{
    return PairContainer<K, V>(m_keys.typedData(), m_values.typedData(), size());
}

//--

END_BOOMER_NAMESPACE(base)
