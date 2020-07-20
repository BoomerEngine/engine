/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "array.h"

namespace base
{
    /// Ordered hash map
    /*
        There are some key things:
         - keys and values are stored in separate arrays to allow for fast iteration
         - 

        Most of the Add/Set/Find function returns pointer directly to the key-value pair data so care must be taken not to modify the map while the pointer is in use.
        Removal of the element from hash map may change the order in the key/value key pair and is in general more expensive than addition
        NOTE: very small hashmaps (<16) elements are do not create the buckets and are used as simple arrays with linear search
        NOTE: we try to be a proper hashmap therefore the number of buckets is NextPow2(size())
    */
    template< class K, class V >
    class HashMap
    {
    public:
        HashMap() = default;
        HashMap(const HashMap<K, V> &other) = default;
        HashMap(HashMap<K, V>&& other) = default;
        HashMap& operator=(const HashMap<K, V> &other) = default;
        HashMap& operator=(HashMap<K, V>&& other) = default;
        ~HashMap() = default;

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
        V *find(const FK& key);

        //! Find value in a safe way
        template< typename FK >
        const V& findSafe(const FK& key, const V& defaultValue = V()) const;

        //! Add key/value pairs from other hashmap into this one
        //! NOTE: values associated with local keys will be replaced with incoming values
        void append(const HashMap<K,V>& other);

        //! Find value by key (read only version), returns pointer to the value (inside the map)
        //! NOTE: the value may not be modified
        template< typename FK >
        const V* find(const FK& key) const;

        //! Find value by key
        template< typename FK >
        bool find(const FK& key, V &output) const;

        //! Test if the map contains a value for given key
        template< typename FK >
        bool contains(const FK& key) const;

        //! Get entry value, if entry does not exist in the map an empty entry is created
        V& operator[](const K& key);

        //! Get entry value, if entry does not exist in the map we fatal assert
        const V& operator[](const K& key) const;

        ///--

        //! Perform operation on each element's key-value pair
        template < typename Func >
        void forEach(const Func& func) const;

        //! Perform non-const operation on each element's key-value value
        //! NOTE: the values are allowed to be modified
        template < typename Func >
        void forEach(const Func& func);

        //! Remove all elements matching given functor
        //! NOTE: returns true if anything was removed
        template < typename Func >
        bool removeIf(const Func& func);

        ///---

        //! Get the array with values only
        Array<V>& values() { return m_values; }

        //! Get the array with values only
        const Array<V>& values() const { return m_values; }

        //! Get the array with keys only
        const Array<K>& keys() const { return m_keys; }

    protected:
        //! rebuild internal bucket table and link table
        //! NOTE: if the number of elements is less than threshold than the tables are removed
        void rehash();

        //! calculate bucket index for element
        template< typename FK >
        uint32_t calcBucketIndex(const FK& key) const;

        //! calculate number of buckets for given array capacity
        static uint32_t BucketsForCapacity(uint32_t capacity);

    private:
        static const uint32_t MIN_ELEMENTS_FOR_HASHING = 16;

        // Key/Value tables
        Array<K> m_keys;
        Array<V> m_values;

        // Next elements
        Array<int> m_links;

        // Hash buckets
        Array<int> m_buckets;
    };

} // base

#include "hashMap.inl"