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
#include "hashBuckets.h"

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
        HashSet(const HashSet<K> &other);
        HashSet(HashSet<K> &&other);
        HashSet& operator=(const HashSet<K> &other);
        HashSet& operator=(HashSet<K> &&other);
        ~HashSet();

        //--

        //! Clear the whole hash set
        void clear();

        //! Reset the hash set without freeing memory
        void reset();

        // Is the hash set empty ?
        bool empty() const;

        // Reserve space in the hash set
        void reserve(uint32_t size);

        // Get number of elements in the hash set
        uint32_t size() const;

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
        Array<K> m_keys;
        HashBuckets* m_buckets = nullptr;
    };

} // base

#include "hashSet.inl"