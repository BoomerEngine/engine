/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "hash.inl"

namespace base
{
    template< class K, class V >
    INLINE void HashMap<K,V>::clear()
    {
        m_keys.clear();
        m_values.clear();
        m_buckets.clear();
        m_links.clear();
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
        m_keys.reserve(size);
        m_values.reserve(size);

        if (size > MIN_ELEMENTS_FOR_HASHING)
        {
            m_links.reserve(size);
            m_buckets.reserve(BucketsForCapacity(size));
        }
    }

    template< class K, class V >
    INLINE void HashMap<K,V>::reset()
    {
        m_keys.reset();
        m_values.reset();
        m_links.reset();

        for (auto& it : m_buckets)
            it = INDEX_NONE;
    }

    template< class K, class V >
    INLINE uint32_t HashMap<K,V>::BucketsForCapacity(uint32_t capacity)
    {
        if (capacity < MIN_ELEMENTS_FOR_HASHING)
            return 0;

        auto x = capacity;

        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return ++x;
    }

    template< class K, class V >
    template< typename FK >
    INLINE uint32_t HashMap<K,V>::calcBucketIndex(const FK& key) const
    {
        auto numBuckets = m_buckets.size();
        ASSERT(numBuckets != 0);
        ASSERT((numBuckets & (numBuckets-1)) == 0);
        auto hash = Hasher<K>::CalcHash(key) & (numBuckets - 1);
        return (uint32_t)hash; // note: NO range cast
    }

    template< class K, class V >
    V* HashMap<K,V>::set(const K& key, const V& val)
    {
        // check existing stuff
        if (m_buckets.empty())
        {
            ASSERT(m_keys.size() < MIN_ELEMENTS_FOR_HASHING);

            // use existing slot
            auto index = m_keys.find(key);
            if (index != INDEX_NONE)
            {
                m_values[index] = val;
                return &m_values[index];
            }

            // add new element
            m_keys.emplaceBack(key);
            m_values.emplaceBack(val);

            // create the hashing table
            if (m_keys.size() == MIN_ELEMENTS_FOR_HASHING)
                rehash();

            // return pointer to placed element
            return &m_values.back();
        }

        // check buckets
        auto bucketIndex = calcBucketIndex(key);
        {
            auto entryIndex = m_buckets[bucketIndex];
            while (entryIndex != INDEX_NONE)
            {
                auto &existingKey = m_keys[entryIndex];
                if (existingKey == key)
                {
                    m_values[entryIndex] = val;
                    return &m_values[entryIndex];
                }

                entryIndex = m_links[entryIndex];
            }
        }

        // create new element
        auto index = range_cast<int>(m_keys.size());
        m_keys.emplaceBack(key);
        m_values.emplaceBack(val);

        // link
        m_links.emplaceBack(m_buckets[bucketIndex]);
        m_buckets[bucketIndex] = index;

        // rehash
        if (m_keys.size() >= m_buckets.size())
            rehash();

        return &m_values.back();
    }

    template< class K, class V >
    template< typename FK >
    bool HashMap<K,V>::remove(const FK& key, V* outRemovedValue /*= nullptr*/)
    {
        // remove in a simple linear array case
        if (m_buckets.empty())
        {
            auto index = m_keys.find(key);
            if (index != INDEX_NONE)
            {
                // extract value being removed
                if (outRemovedValue)
                    *outRemovedValue = std::move(m_values[index]);

                m_keys.eraseUnordered(index);
                m_values.eraseUnordered(index);
                return true;
            }
            else
            {
                return false;
            }
        }

        // we are going to search for crap in the list and remove it
        auto bucketIndex = calcBucketIndex(key);
        int* prevPtr = &m_buckets[bucketIndex];
        while (*prevPtr != INDEX_NONE)
        {
            auto entryIndex = *prevPtr;
            if (m_keys[entryIndex] == key)
            {
                // unlink from array
                *prevPtr = m_links[entryIndex];

                // get the index of the last element in the key/value tables
                // this element will have to be reindexed
                auto lastEntryIndex = m_keys.lastValidIndex();

                // extract value being removed
                if (outRemovedValue)
                    *outRemovedValue = std::move(m_values[entryIndex]);

                // erase the removed element
                m_keys.eraseUnordered(entryIndex);
                m_values.eraseUnordered(entryIndex);
                m_links.eraseUnordered(entryIndex);

                // since we moved an element from index lastEntryIndex to entryIndex change all the stuff in the tables
                if (lastEntryIndex != entryIndex)
                {
                    auto& lastEntryKey = m_keys[entryIndex]; // it's now moved
                    auto lastEntryBucketIndex = calcBucketIndex(lastEntryKey);

                    auto updateIndex  = &m_buckets[lastEntryBucketIndex];
                    while (*updateIndex != INDEX_NONE)
                    {
                        if (*updateIndex == lastEntryIndex)
                        {
                            *updateIndex = entryIndex;
                            break;
                        }

                        updateIndex = &m_links[*updateIndex];
                    }
                }

                // item was removed
                return true;
            }

            // go to next item
            prevPtr = &m_links[entryIndex];
        }

        // item was not removed
        return false;
    }

    template< class K, class V >
    template< typename FK >
    V* HashMap<K, V>::find(const FK& key)
    {
        // simple linear search
        if (m_buckets.empty())
        {
            auto index = m_keys.find(key);
            return (index != INDEX_NONE) ? &m_values[index] : nullptr;
        }

        // hash search
        auto bucketIndex = calcBucketIndex(key);
        auto entryIndex = m_buckets[bucketIndex];
        while (entryIndex != INDEX_NONE)
        {
            auto &existingKey = m_keys[entryIndex];
            if (existingKey == key)
                return &m_values[entryIndex];

            entryIndex = m_links[entryIndex];
        }

        // not found
        return nullptr;
    }

    template< class K, class V >
    template< typename FK >
    const V* HashMap<K,V>::find(const FK& key) const
    {
        // simple linear search
        if (m_buckets.empty())
        {
            auto index = m_keys.find(key);
            return (index != INDEX_NONE) ? &m_values[index] : nullptr;
        }

        // hash search
        auto bucketIndex = calcBucketIndex(key);
        auto entryIndex = m_buckets[bucketIndex];
        while (entryIndex != INDEX_NONE)
        {
            auto &existingKey = m_keys[entryIndex];
            if (existingKey == key)
                return &m_values[entryIndex];

            entryIndex = m_links[entryIndex];
        }

        // not found
        return nullptr;
    }

    template< class K, class V >
    template< typename FK >
    INLINE bool HashMap<K,V>::find(const FK& key, V &output) const
    {
        auto val = find(key);
        if (val)
        {
            output = *val;
            return true;
        }

        return false;
    }

    template< class K, class V >
    template< typename FK >
    INLINE const V& HashMap<K,V>::findSafe(const FK& key, const V& defaultValue) const
    {
        auto val = find(key);
        return val ? *val : defaultValue;
    }

    template< class K, class V >
    template< typename FK >
    INLINE bool HashMap<K,V>::contains(const FK& key) const
    {
        return find(key);
    }

    template< class K, class V >
    void HashMap<K,V>::append(const HashMap<K,V>& other)
    {
        auto size = other.m_keys.size();
        auto keyPtr  = other.m_keys.typedData();
        auto valuePtr  = other.m_values.typedData();
        for (uint32_t i=0; i<size; ++i, ++keyPtr, ++valuePtr)
            set(*keyPtr, *valuePtr);
    }

    template< class K, class V >
    INLINE V& HashMap<K,V>::operator[](const K& key)
    {
        auto ptr  = find(key);
        if (!ptr)
            ptr = set(key, V());
        return *ptr;
    }

    template< class K, class V >
    INLINE const V& HashMap<K,V>::operator[](const K& key) const
    {
        auto ptr  = find(key);
        ASSERT_EX(ptr, "Element not found in map even though it was strongly expected");
        return *ptr;
    }

    template< class K, class V >
    template < typename Func >
    INLINE void HashMap<K,V>::forEach(const Func& func) const
    {
        auto keyPtr  = m_keys.typedData();
        auto keyEndPtr  = keyPtr + m_keys.size();
        auto valuePtr  = m_values.typedData();
        while (keyPtr < keyEndPtr)
            func(*keyPtr++, *valuePtr++);
    }

    template< class K, class V >
    template < typename Func >
    INLINE void HashMap<K,V>::forEach(const Func& func)
    {
        auto keyPtr  = m_keys.typedData();
        auto keyEndPtr  = keyPtr + m_keys.size();
        auto valuePtr  = m_values.typedData();
        while (keyPtr < keyEndPtr)
            func(*keyPtr++, *valuePtr++);
    }

    template< class K, class V >
    template < typename Func >
    INLINE bool HashMap<K,V>::removeIf(const Func& func)
    {
        bool stuffRemoved = false;
        for (int i=m_values.lastValidIndex(); i >= 0; --i)
        {
            if (func(m_keys[i], m_values[i]))
            {
                stuffRemoved = true;
                remove(m_keys[i]);
            }
        }

        return stuffRemoved;
    }

    template< class K, class V >
    void HashMap<K,V>::rehash()
    {
        auto numBuckets = BucketsForCapacity(m_keys.capacity());
        if (numBuckets != m_buckets.size())
        {
            // resize the bucket table
            m_buckets.resize(numBuckets);
            for (auto& it : m_buckets)
                it = INDEX_NONE;

            // reset the links to
            m_links.reset();

            // reinsert the keys
            if (!m_buckets.empty())
            {
                auto keyPtr = m_keys.typedData();
                auto size = m_keys.size();
                for (uint32_t keyIndex = 0; keyIndex < size; ++keyIndex)
                {
                    auto bucketIndex = calcBucketIndex(keyPtr[keyIndex]);
                    m_links.emplaceBack(m_buckets[bucketIndex]);
                    m_buckets[bucketIndex] = (int) keyIndex;
                }
            }
        }
    }

} // base
