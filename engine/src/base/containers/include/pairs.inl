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
    ALWAYS_INLINE ConstPairIterator<K, V>::ConstPairIterator()
    {}

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V>::ConstPairIterator(const K* keys, const V* values)
        : m_keyPtr(keys)
        , m_valuePtr(values)
    {}

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K,V>::ConstPairIterator(const ConstPairIterator<K,V>& other)
        : m_keyPtr(other.m_keyPtr, other.m_valuePtr)
    {}

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V>& ConstPairIterator<K, V>::operator++()
    {
        ++m_keyPtr;
        ++m_valuePtr;
        return *this;
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V>& ConstPairIterator<K, V>::operator--()
    {
        --m_keyPtr;
        --m_valuePtr;
        return *this;
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V> ConstPairIterator<K, V>::operator++(int)
    {
        ConstPairIterator<K, V> ret;
        ++ret.m_keyPtr;
        ++ret.m_valuePtr;
        return ret;
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V> ConstPairIterator<K, V>::operator--(int)
    {
        ConstPairIterator<K, V> ret;
        --ret.m_keyPtr;
        --ret.m_valuePtr;
        return ret;
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairRef<K, V> ConstPairIterator<K, V>::operator*() const
    {
        return ConstPairRef<K, V>(*m_keyPtr, *m_valuePtr);
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairRef<K, V> ConstPairIterator<K, V>::operator->() const
    {
        return ConstPairRef<K, V>(*m_keyPtr, *m_valuePtr);
    }

    template< class K, class V >
    ALWAYS_INLINE ptrdiff_t ConstPairIterator<K, V>::operator-(const ConstPairIterator<K, V>& other) const
    {
        return m_keyPtr - other.m_keyPtr;
    }

    template< class K, class V >
    ALWAYS_INLINE bool ConstPairIterator<K, V>::operator==(const ConstPairIterator<K, V >& other) const
    {
        return (m_keyPtr == other.m_keyPtr) && (m_valuePtr == other.m_valuePtr);
    }

    template< class K, class V >
    ALWAYS_INLINE bool ConstPairIterator<K, V>::operator!=(const ConstPairIterator<K, V>& other) const
    {
        return (m_keyPtr != other.m_keyPtr) || (m_valuePtr != other.m_valuePtr);
    }

    template< class K, class V >
    ALWAYS_INLINE bool ConstPairIterator<K, V>::operator<(const ConstPairIterator<K, V>& other) const
    {
        return m_keyPtr < other.m_keyPtr;
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V> ConstPairIterator<K, V>::operator+(const ptrdiff_t diff) const
    {
        ConstPairIterator<K, V> ret(*this);
        ret.m_keyPtr += diff;
        ret.m_valuePtr += diff;
        return ret;
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V>& ConstPairIterator<K, V>::operator+=(const ptrdiff_t diff)
    {
        m_keyPtr += diff;
        m_valuePtr += diff;
        return *this;
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V> ConstPairIterator<K, V>::operator-(const ptrdiff_t diff) const
    {
        ConstPairIterator<K, V> ret(*this);
        ret.m_keyPtr -= diff;
        ret.m_valuePtr -= diff;
        return ret;
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V>& ConstPairIterator<K, V>::operator-=(const ptrdiff_t diff)
    {
        m_keyPtr -= diff;
        m_valuePtr -= diff;
        return *this;
    }

    //---

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V>::PairIterator()
    {}

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V>::PairIterator(const K* keys, V* values)
        : m_keyPtr(keys)
        , m_valuePtr(values)
    {}

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V>::PairIterator(const PairIterator<K, V>& other)
        : m_keyPtr(other.m_keyPtr, other.m_valuePtr)
    {}

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V>& PairIterator<K, V>::operator++()
    {
        ++m_keyPtr;
        ++m_valuePtr;
        return *this;
    }

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V>& PairIterator<K, V>::operator--()
    {
        --m_keyPtr;
        --m_valuePtr;
        return *this;
    }

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V> PairIterator<K, V>::operator++(int)
    {
        PairIterator<K, V> ret;
        ++ret.m_keyPtr;
        ++ret.m_valuePtr;
        return ret;
    }

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V> PairIterator<K, V>::operator--(int)
    {
        PairIterator<K, V> ret;
        --ret.m_keyPtr;
        --ret.m_valuePtr;
        return ret;
    }

    template< class K, class V >
    ALWAYS_INLINE PairRef<K, V> PairIterator<K, V>::operator*() const
    {
        return PairRef<K, V>(*m_keyPtr, *m_valuePtr);
    }

    template< class K, class V >
    ALWAYS_INLINE PairRef<K, V> PairIterator<K, V>::operator->() const
    {
        return PairRef<K, V>(*m_keyPtr, *m_valuePtr);
    }

    template< class K, class V >
    ALWAYS_INLINE ptrdiff_t PairIterator<K, V>::operator-(const PairIterator<K, V>& other) const
    {
        return m_keyPtr - other.m_keyPtr;
    }

    template< class K, class V >
    ALWAYS_INLINE bool PairIterator<K, V>::operator==(const PairIterator<K, V >& other) const
    {
        return (m_keyPtr == other.m_keyPtr) && (m_valuePtr == other.m_valuePtr);
    }

    template< class K, class V >
    ALWAYS_INLINE bool PairIterator<K, V>::operator!=(const PairIterator<K, V>& other) const
    {
        return (m_keyPtr != other.m_keyPtr) || (m_valuePtr != other.m_valuePtr);
    }

    template< class K, class V >
    ALWAYS_INLINE bool PairIterator<K, V>::operator<(const PairIterator<K, V>& other) const
    {
        return m_keyPtr < other.m_keyPtr;
    }

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V> PairIterator<K, V>::operator+(const ptrdiff_t diff) const
    {
        PairIterator<K, V> ret(*this);
        ret.m_keyPtr += diff;
        ret.m_valuePtr += diff;
        return ret;
    }

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V>& PairIterator<K, V>::operator+=(const ptrdiff_t diff)
    {
        m_keyPtr += diff;
        m_valuePtr += diff;
        return *this;
    }

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V> PairIterator<K, V>::operator-(const ptrdiff_t diff) const
    {
        PairIterator<K, V> ret(*this);
        ret.m_keyPtr -= diff;
        ret.m_valuePtr -= diff;
        return ret;
    }

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V>& PairIterator<K, V>::operator-=(const ptrdiff_t diff)
    {
        m_keyPtr -= diff;
        m_valuePtr -= diff;
        return *this;
    }

    //---

    template< class K, class V >
    ALWAYS_INLINE PairContainer<K,V>::PairContainer()
    {}

    template< class K, class V >
    ALWAYS_INLINE PairContainer<K, V>::PairContainer(const K* keys, const V* values, uint32_t size)
        : m_keys(keys)
        , m_values(values)
        , m_size(size)
    {
        if (m_size == 0)
        {
            DEBUG_CHECK(m_keys == nullptr);
            DEBUG_CHECK(m_values == nullptr);
        }
        else
        {
            DEBUG_CHECK(m_keys != nullptr);
            DEBUG_CHECK(m_values != nullptr);
        }
    }

    template< class K, class V >
    ALWAYS_INLINE PairContainer<K, V>::PairContainer(PairContainer<K, V>&& other)
    {
        m_keys = other.m_keys;
        m_values = other.m_values;
        m_size = other.m_size;
        other.m_keys = nullptr;
        other.m_values = nullptr;
        other.m_size = 0;
    }

    template< class K, class V >
    ALWAYS_INLINE PairContainer<K, V>& PairContainer<K,V>::operator=(PairContainer<K, V>&& other)
    {
        if (this != &other)
        {
            m_keys = other.m_keys;
            m_values = other.m_values;
            m_size = other.m_size;
            other.m_keys = nullptr;
            other.m_values = nullptr;
            other.m_size = 0;
        }

        return *this;
    }

    template< class K, class V >
    ALWAYS_INLINE uint32_t PairContainer<K, V>::size() const
    {
        return m_size;
    }

    template< class K, class V >
    ALWAYS_INLINE IndexRange PairContainer<K, V>::indices() const
    {
        return IndexRange(0, m_size);
    }

    template< class K, class V >
    ALWAYS_INLINE bool PairContainer<K, V>::empty() const
    {
        return m_size == 0;
    }

    template< class K, class V >
    ALWAYS_INLINE PairContainer<K, V>::operator bool() const
    {
        return m_size != 0;
    }

    template< class K, class V >
    ALWAYS_INLINE const K* PairContainer<K, V>::keysData() const
    {
        return m_keys;
    }

    template< class K, class V >
    ALWAYS_INLINE const V* PairContainer<K, V>::valuesData() const
    {
        return m_values;
    }

    template< class K, class V >
    ALWAYS_INLINE V* PairContainer<K, V>::valuesData()
    {
        return m_values;
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairRef<K, V> PairContainer<K, V>::operator[](uint32_t index) const
    {
        DEBUG_CHECK(index < m_size);
        return ConstPairRef<K, V>(m_keys[index], m_values[index]);
    }

    template< class K, class V >
    ALWAYS_INLINE PairRef<K, V> PairContainer<K, V>::operator[](uint32_t index)
    {
        DEBUG_CHECK(index < m_size);
        return PairRef<K, V>(m_keys[index], m_values[index]);
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V> PairContainer<K, V>::begin() const
    {
        return ConstPairIterator<K, V>(m_keys, m_values);
    }

    template< class K, class V >
    ALWAYS_INLINE ConstPairIterator<K, V> PairContainer<K, V>::end() const
    {
        return ConstPairIterator<K, V>(m_keys + m_size, m_values + m_size);
    }

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V> PairContainer<K, V>::begin()
    {
        return PairIterator<K, V>(m_keys, const_cast<V*>(m_values));
    }

    template< class K, class V >
    ALWAYS_INLINE PairIterator<K, V> PairContainer<K, V>::end()
    {
        return PairIterator<K, V>(m_keys + m_size, const_cast<V*>(m_values) + m_size);
    }

    //---

} // base
