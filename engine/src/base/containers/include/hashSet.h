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

namespace base
{
    /// Hashed set
    /*
        Allows fast insert and removal and O(1) amortized search
        Keys are stored in linear table to allow iteration
        NOTE: duplicate keys are stored only once   
    */
    template< class K >
    class HashSet
    {
    public:
        HashSet() = default;
        HashSet(const HashSet<K> &other) = default;
        HashSet(HashSet<K> &&other) = default;
        HashSet& operator=(const HashSet<K> &other) = default;
        HashSet& operator=(HashSet<K> &&other) = default;
        ~HashSet() = default;

        //! Clear the whole hash set
        void clear();

        //! Reset the hash set without freeing memory
        void reset();

        // Is the hash set empty ?
        bool empty() const;

        // Get number of elements in the hash set
        uint32_t size() const;

        // Reserve space in the hash set
        void reserve(uint32_t size);

        //---

        //! Insert key into the set
        //! NOTE: Returns true if key was added to the set, false if it already exists in the set
        bool insert(const K& key);

        //! Remove key from set
        //! NOTE: Returns true if key was removed to the set, false if it was not in the set
        template< typename FK >
        bool remove(const FK& key);

        //! Check if set contains given value
        template< typename FK >
        bool contains(const FK& key) const;

        //----

        //! Get the array with keys only
        const Array<K>& keys() const { return m_keys; }

        //! Get read only iterator to start of the array
        ConstArrayIterator<K> begin() const { return m_keys.begin(); }

        //! Get read only iterator to end of the array
        ConstArrayIterator<K> end() const { return m_keys.end(); }

        //----

    protected:
        void removeKey(int keyIndex, uint32_t hashIndex);
        void changeIndex(int oldIndex, int newIndex, uint32_t oldHashIndex, uint32_t newHashIndex);
        void removeIndex(int oldIndex, uint32_t hashIndex);

        void validate();
        void rehash();

        //--

        static const uint32_t kMinKeysForHashing = 32; // until we get that many keys we are using the linear array

        static uint32_t CalcOptimalBucketCount(uint32_t numKeys);

        struct Entry
        {
            int next = INDEX_NONE;
            int prev = INDEX_NONE;
        };

        typedef Array<K> TKeyList;
        TKeyList m_keys;

        typedef Array<Entry> TLinkList;
        TLinkList m_links;

        typedef Array<int> THashBuckets;
        THashBuckets m_buckets;
    };

} // base

#include "hashSet.inl"