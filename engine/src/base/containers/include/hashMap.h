/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "array.h"
#include "hashBuckets.h"
#include "pairs.h"

namespace base
{
    ///--

    /// Hash map with directly accessible keys() and values() arrays
    template< class K, class V >
    class HashMap
    {
    public:
        HashMap() = default;
        HashMap(const HashMap<K, V>& other);
        HashMap(HashMap<K, V>&& other);
        HashMap& operator=(const HashMap<K, V>& other);
        HashMap& operator=(HashMap<K, V>&& other);
        ~HashMap();

        //! Clear the whole hash map
        /// NOTE: all memory is released, use reset() to preserve the memory
        void clear();

        //! Delete (via MemDelete) the children and clear the map
        //! NOTE: will not compile if V is not a pointer
        //! NOTE: the V elements should be allocated with MemNew
        void clearPtr();

        //! Clear the whole hash map without freeing the memory
        void reset();

        //! Is the hash map empty ?
        bool empty() const;

        //! Get number of elements in the hash map
        uint32_t size() const;;

        //! Reserve space in the hash map
        //! NOTE: reserves the space for the buckets only if the element count is over the threshold
        void reserve(uint32_t size);

        ///--

        //! Set/Create value for given key, returns pointer to the value (inside the map)
        //! NOTE: if the value for given key is already defined than we change the existing element
        V* set(const K& key, const V& val);

        //! Remove key from map, returns true if the element was removed and also returns the value of the element removed
        template< typename FK >
        bool remove(const FK& key, V* outRemovedValue = nullptr);

        //! Find value by key, returns pointer to the value (inside the map)
        //! NOTE: the value may be modified freely
        template< typename FK >
        V* find(const FK& key);

        //! Find value in a safe way
        template< typename FK >
        const V& findSafe(const FK& key, const V& defaultValue = V()) const;

        //! Add key/value pairs from other hashmap into this one
        //! NOTE: values associated with local keys will be replaced with incoming values
        void append(const HashMap<K, V>& other);

        //! Find value by key (read only version), returns pointer to the value (inside the map)
        //! NOTE: the value may not be modified
        template< typename FK >
        const V* find(const FK& key) const;

        //! Find value by key
        template< typename FK >
        bool find(const FK& key, V& output) const;

        //! Test if the map contains a value for given key
        template< typename FK >
        bool contains(const FK& key) const;

        //----

        //! Get entry value, if entry does not exist in the map an empty entry is created
        V& operator[](const K& key);

        //! Get entry value, if entry does not exist in the map we fatal assert
        const V& operator[](const K& key) const;

        //---

        //! Get the array with values only
        Array<V>& values();

        //! Get the array with values only
        const Array<V>& values() const;

        //! Get the array with keys only
        const Array<K>& keys() const;

        //! Get table of pairs
        const PairContainer<K, V> pairs() const;

        //! Get table of pairs
        PairContainer<K, V> pairs();

    protected:
        Array<K> m_keys;
        Array<V> m_values;

        HashBuckets* m_buckets = nullptr;

        //--

        V* add(const K& key, const V& val);
    };

    ///--

} // base

#include "hashMap.inl"