/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

namespace base
{
    //---

    template< class K, class V >
    struct PairRef : public NoCopy
    {
        const K& key;
        V& value;

        ALWAYS_INLINE PairRef(const K& k, V& v)
            : key(k), value(v)
        {}

        ALWAYS_INLINE PairRef(PairRef&& other)
            : key(other.key), value(other.value)
        {}
    };

    template< class K, class V >
    struct ConstPairRef : public NoCopy
    {
        const K& key;
        const V& value;

        ALWAYS_INLINE ConstPairRef(const K& k, const V& v)
            : key(k), value(v)
        {}

        ALWAYS_INLINE ConstPairRef(ConstPairRef&& other)
            : key(other.key), value(other.value)
        {}
    };

    //--

    // for (auto pair : x.pairs()) iterator
    template< class K, class V >
    class ConstPairIterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = ConstPairRef<K,V>;
        using difference_type = ptrdiff_t;
        using pointer = ConstPairRef<K, V>;
        using reference = ConstPairRef<K, V>;

        ConstPairIterator();
        ConstPairIterator(const K* keys, const V* values);
        ConstPairIterator(const ConstPairIterator<K,V>& other);

        ConstPairIterator<K, V>& operator++();
        ConstPairIterator<K, V>& operator--();
        ConstPairIterator<K, V> operator++(int);
        ConstPairIterator<K, V> operator--(int);

        ConstPairRef<K, V> operator*() const;
        ConstPairRef<K, V> operator->() const;

        ptrdiff_t operator-(const ConstPairIterator<K, V>& other) const;

        bool operator==(const ConstPairIterator<K, V >& other) const;
        bool operator!=(const ConstPairIterator<K, V>& other) const;
        bool operator<(const ConstPairIterator<K, V>& other) const;

        ConstPairIterator<K, V> operator+(const ptrdiff_t diff) const;
        ConstPairIterator<K, V>& operator+=(const ptrdiff_t diff);

        ConstPairIterator<K, V> operator-(const ptrdiff_t diff) const;
        ConstPairIterator<K, V>& operator-=(const ptrdiff_t diff);

    private:
        const K* m_keyPtr = nullptr;
        const V* m_valuePtr = nullptr;
    };

    //--

    // for (auto pair : x.pairs()) iterator
    template< class K, class V >
    class PairIterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = PairRef<K, V>;
        using difference_type = ptrdiff_t;
        using pointer = PairRef<K, V>;
        using reference = PairRef<K, V>;

        PairIterator();
        PairIterator(const K* keys, V* values);
        PairIterator(const PairIterator<K, V>& other);

        PairIterator<K, V>& operator++();
        PairIterator<K, V>& operator--();
        PairIterator<K, V> operator++(int);
        PairIterator<K, V> operator--(int);

        PairRef<K, V> operator*() const;
        PairRef<K, V> operator->() const;

        ptrdiff_t operator-(const PairIterator<K, V>& other) const;

        bool operator==(const PairIterator<K, V >& other) const;
        bool operator!=(const PairIterator<K, V>& other) const;
        bool operator<(const PairIterator<K, V>& other) const;

        PairIterator<K, V> operator+(const ptrdiff_t diff) const;
        PairIterator<K, V>& operator+=(const ptrdiff_t diff);

        PairIterator<K, V> operator-(const ptrdiff_t diff) const;
        PairIterator<K, V>& operator-=(const ptrdiff_t diff);

    private:
        const K* m_keyPtr = nullptr;
        V* m_valuePtr = nullptr;
    };

    //--

    /// general (temporary) container-like wrapper for stuff with pair
    template< class K, class V >
    class PairContainer : public NoCopy
    {
    public:
        PairContainer();
        PairContainer(const K* keys, const V* values, uint32_t size);
        PairContainer(PairContainer<K, V>&& other);
        PairContainer& operator=(PairContainer<K, V>&& other);

        uint32_t size() const;
        IndexRange indices() const;

        bool empty() const;
        operator bool() const;

        const K* keysData() const;
        const V* valuesData() const;
        V* valuesData();

        ConstPairRef<K,V> operator[](uint32_t index) const;
        PairRef<K, V> operator[](uint32_t index);

        ConstPairIterator<K, V> begin() const;
        ConstPairIterator<K, V> end() const;

        PairIterator<K, V> begin();
        PairIterator<K, V> end();

    private:
        const K* m_keys = nullptr;
        const V* m_values = nullptr;

        uint32_t m_size = 0;
    };

    //--

} // base

#include "pairs.inl"