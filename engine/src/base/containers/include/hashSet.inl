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

namespace base
{
    template<typename K>
    INLINE void HashSet<K>::clear()
    {
        m_keys.clear();
        m_buckets.clear();
        m_links.clear();
    }

    template<typename K>
    INLINE void HashSet<K>::reset()
    {
        m_keys.reset();
        m_links.reset();
        m_buckets.reset();
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
        m_keys.reserve(size);
        m_links.reserve(size);
        rehash();
    }

    template<typename K>
    bool HashSet<K>::insert(const K& key)
    {
        // compute hash
        auto rawHash = Hasher<K>::CalcHash(key);

        // insert
        if (m_buckets.empty())
        {
            // linear check
            for (auto& curKey : m_keys)
                if (curKey == key)
                    return false;
        }
        else
        {
            // Get hash value
            auto hashCount = m_buckets.size();
            auto hash = (uint32_t)(rawHash & (hashCount - 1));

            // Find matching
            int next = INDEX_NONE;
            int prev = INDEX_NONE;
            for (auto i = m_buckets[hash]; i != INDEX_NONE; prev = i, i = next)
            {
                auto& curKey = m_keys[i];
                if (curKey == key)
                    return false; // key already exists

                // Go to next element in hash
                DEBUG_CHECK(prev == m_links[i].prev);
                next = m_links[i].next;
            }
        }

        // add the entry
        auto index = range_cast<int>(m_keys.size());
        m_keys.emplaceBack(key);

        // enough keys ?
        auto optimalBucketCount = CalcOptimalBucketCount(m_keys.capacity());
        if (optimalBucketCount > 0)
        {
            // rehash
            if (m_buckets.size() != optimalBucketCount)
            {
                rehash();
            }
            else
            {
                // Get hash value
                auto hashCount = m_buckets.size();
                auto hash = (uint32_t) (rawHash & (hashCount - 1));

                auto curLink = m_buckets[hash];
                if (curLink != INDEX_NONE)
                {
                    ASSERT(m_links[curLink].prev == INDEX_NONE);
                    m_links[curLink].prev = index;
                }
                m_links.emplaceBack().next = curLink;
                m_buckets[hash] = index;
            }
        }

        // key was added
        ASSERT(m_buckets.empty() || m_keys.size() == m_links.size());
        return true;
    }

    template<typename K>
    template< typename FK >
    bool HashSet<K>::remove(const FK& key)
    {
        // no buckets, linear array
        if (m_buckets.empty())
        {
            auto numRemoved = m_keys.remove(key);
            return (numRemoved != 0);
        }

        // compute hash
        auto rawHash = Hasher<K>::CalcHash(key);

        // Get hash value
        auto hashCount = m_buckets.size();
        auto hash = (uint32_t)(rawHash & (hashCount - 1));

        // Find matching
        int next = INDEX_NONE;
        int prev = INDEX_NONE;
        for (auto i = m_buckets[hash]; i != INDEX_NONE; prev = i, i = next)
        {
            auto& curKey = m_keys[i];
            if (curKey == key)
            {
                removeKey(i, hash);
                return true;
            }

            // Go to next element in hash
            DEBUG_CHECK(prev == m_links[i].prev);
            next = m_links[i].next;
        }

        // Key was not found
        return false;
    }

    template<typename K>
    template< typename FK >
    bool HashSet<K>::contains(const FK& key) const
    {
        // empty buckets, check in linear way
        if (m_buckets.empty())
        {
            return m_keys.contains(key);
        }
        else
        {
            // Get hash using hasher and translate that into bucket number
            auto hashCount = m_buckets.size();
            auto hash = Hasher<K>::CalcHash(key) & (hashCount - 1);

            // Search
            int next = INDEX_NONE;
            int prev = INDEX_NONE;
            for (auto i = m_buckets[hash]; i != INDEX_NONE; prev = i, i = next)
            {
#ifdef _DEBUG
                uint32_t keyHash = Hasher<K>::CalcHash(key) & (hashCount - 1);
                DEBUG_CHECK(keyHash == hash);
#endif

                if (m_keys[i] == key)
                    return true;

                DEBUG_CHECK(m_links[i].prev == prev);
                next = m_links[i].next;
            }
        }

        // Not found
        return false;
    }

    template<typename K>
    void HashSet<K>::removeKey(int keyIndex, uint32_t hashIndex)
    {
        // remove the mention of the shifted entry
        removeIndex(keyIndex, hashIndex);

        // swap keys
        auto lastKeyIndex = m_keys.lastValidIndex();
        if (keyIndex < lastKeyIndex)
        {
            // move the last key to a new place
            std::swap(m_keys[keyIndex], m_keys[lastKeyIndex]);

            // calculate new hash index
            auto hashCount = m_buckets.size();
            auto newHashIndex = Hasher<K>::CalcHash(m_keys[keyIndex]) & (hashCount - 1);

            // change the indices
            changeIndex(lastKeyIndex, keyIndex, hashIndex, newHashIndex);
        }

        // reduce array
        m_keys.popBack();
        m_links.popBack();

        // we got under the threshold of hashing
        auto optimalCount = CalcOptimalBucketCount(m_keys.capacity());
        if (optimalCount != m_buckets.size())
            rehash();

        // validate structure of the hash set
        ASSERT(m_buckets.empty() || m_keys.size() == m_links.size());
        validate();
    }

    template<typename K>
    void HashSet<K>::changeIndex(int oldIndex, int newIndex, uint32_t oldHashIndex, uint32_t newHashIndex)
    {
        auto oldPrev = m_links[oldIndex].prev;
        auto oldNext = m_links[oldIndex].next;
        m_links[oldIndex].prev = INDEX_NONE;
        m_links[oldIndex].next= INDEX_NONE;
        m_links[newIndex].prev = oldPrev;
        m_links[newIndex].next= oldNext;

        if (oldPrev != INDEX_NONE)
        {
            ASSERT(m_links[oldPrev].next== oldIndex);
            m_links[oldPrev].next= newIndex;
        }
        else
        {
            ASSERT(m_buckets[newHashIndex] == oldIndex);
            m_buckets[newHashIndex] = newIndex;
        }

        if (oldNext != INDEX_NONE)
        {
            ASSERT(m_links[oldNext].prev == oldIndex);
            m_links[oldNext].prev = newIndex;
        }
    }

    template<typename K>
    void HashSet<K>::removeIndex(int oldIndex, uint32_t hashIndex)
    {
        auto oldPrev = m_links[oldIndex].prev;
        auto oldNext = m_links[oldIndex].next;
        m_links[oldIndex].prev = INDEX_NONE;
        m_links[oldIndex].next= INDEX_NONE;

        if (oldNext != INDEX_NONE)
        {
            ASSERT(m_links[oldNext].prev == oldIndex);
            m_links[oldNext].prev = oldPrev;
        }

        if (oldPrev != INDEX_NONE)
        {
            ASSERT(m_links[oldPrev].next== oldIndex);
            m_links[oldPrev].next= oldNext;
        }
        else
        {
            ASSERT(m_buckets[hashIndex] == oldIndex);
            m_buckets[hashIndex] = oldNext;
        }
    }

    template<typename K>
    void HashSet<K>::validate()
    {
#ifdef BUILD_DEBUG
        auto optimalCount = CalcOptimalBucketCount(m_keys.capacity());
        ASSERT(m_buckets.size() == optimalCount);
        if (optimalCount == 0)
        {
            ASSERT(m_links.empty());
        }
        else
        {
            ASSERT(m_links.size() == m_keys.size());

            for (int i = 0; i < (int)m_links.size(); ++i)
            {
                auto& cur = m_links[i];

                if (cur.prev != INDEX_NONE)
                {
                    ASSERT(m_links[cur.prev].next== i);
                }

                if (cur.next!= INDEX_NONE)
                {
                    ASSERT(m_links[cur.next].prev == i);
                }
            }

            for (int i = 0; i < (int)m_keys.size(); ++i)
            {
                auto hashCount = m_buckets.size();
                auto hash = Hasher<K>::CalcHash(m_keys[i]) & (hashCount - 1);
                auto index = m_buckets[hash];

                bool found = false;
                while (index != INDEX_NONE)
                {
                    if (index == i)
                    {
                        found = true;
                        break;
                    }

                    index = m_links[index].next;
                }

                ASSERT(found);
            }
        }
#endif
    }

    template<typename K>
    void HashSet<K>::rehash()
    {
        // prepare bucket table
        auto numBuckets = CalcOptimalBucketCount(m_keys.capacity());
        m_buckets.resize(numBuckets);

        // link elements
        if (numBuckets > 0)
        {
            // setup links
            m_links.resize(m_keys.size());

            // setup buckets
            for (auto& it : m_buckets)
                it = INDEX_NONE;

            // link elements
            for (int i = 0; i < (int)m_keys.size(); ++i)
            {
                auto keyHash = Hasher<K>::CalcHash(m_keys[i]) & (numBuckets - 1);
                auto prevLink = m_buckets[keyHash];

                if (prevLink != INDEX_NONE)
                {
                    ASSERT(m_links[prevLink].prev == INDEX_NONE);
                    m_links[prevLink].prev = i;
                }

                m_links[i].prev = INDEX_NONE;
                m_links[i].next= prevLink;
                m_buckets[keyHash] = i;
            }
        }
        else
        {
            // no hashing in small arrays
            m_links.reset();
        }
    }

    template<typename K>
    INLINE uint32_t HashSet<K>::CalcOptimalBucketCount(uint32_t numKeys)
    {
        if (numKeys < kMinKeysForHashing)
            return 0;

        auto v = numKeys;
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

} // base
