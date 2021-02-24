/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//--

template< typename T >
ALWAYS_INLINE PointerRange<T>::PointerRange() = default;

template< typename T >
ALWAYS_INLINE PointerRange<T>::PointerRange(T* ptr, uint64_t length)
{
    if (ptr && length)
    {
        m_start = ptr;
        m_end = ptr + length;
    }
}

template< typename T >
ALWAYS_INLINE PointerRange<T>::PointerRange(T* start, T* end)
{
    if (start && end >= start)
    {
        m_start = start;
        m_end = end;
    }
}

template< typename T >
ALWAYS_INLINE PointerRange<T>::PointerRange(const T* ptr, uint64_t length)
{
    if (ptr && length)
    {
        m_start = const_cast<T*>(ptr);
        m_end = m_start + length;
    }
}

template< typename T >
ALWAYS_INLINE PointerRange<T>::PointerRange(const T* start, const T* end)
{
    if (start && end >= start)
    {
        m_start = const_cast<T*>(start);
        m_end = const_cast<T*>(end);
    }
}

template< typename T >
ALWAYS_INLINE PointerRange<T>::PointerRange(const PointerRange<T>& other) = default;

template< typename T >
ALWAYS_INLINE PointerRange<T>::PointerRange(PointerRange<T>&& other)
    : m_start(other.m_start)
    , m_end(other.m_end)
{
    other.m_start = nullptr;
    other.m_end = nullptr;
}

template< typename T >
ALWAYS_INLINE PointerRange<T>& PointerRange<T>::operator=(const PointerRange<T>& other) = default;

template< typename T >
ALWAYS_INLINE PointerRange<T>& PointerRange<T>::operator=(PointerRange<T>&& other)
{
    if (this != &other)
    {
        m_start = other.m_start;
        m_end = other.m_end;
        other.m_start = nullptr;
        other.m_end = nullptr;
    }

    return *this;
}

template< typename T >
ALWAYS_INLINE PointerRange<T>::~PointerRange() = default;

template< typename T >
ALWAYS_INLINE void* PointerRange<T>::data()
{
    return m_start;
}

template< typename T >
ALWAYS_INLINE const void* PointerRange<T>::data() const
{
    return m_start;
}

template< typename T >
ALWAYS_INLINE T* PointerRange<T>::typedData()
{
    return m_start;
}

template< typename T >
ALWAYS_INLINE const T* PointerRange<T>::typedData() const
{
    return m_start;
}

template< typename T >
ALWAYS_INLINE uint64_t PointerRange<T>::dataSize() const
{
    return (m_end - m_start) * sizeof(T);
}

template< typename T >
ALWAYS_INLINE int PointerRange<T>::lastValidIndex() const
{
    return (int)((m_end - m_start) - 1);
}

template< typename T >
ALWAYS_INLINE uint64_t PointerRange<T>::size() const
{
    return m_end - m_start;
}

template< typename T >
ALWAYS_INLINE bool PointerRange<T>::empty() const
{
    return m_end > m_start;
}

template< typename T >
ALWAYS_INLINE BasePointerRange PointerRange<T>::bytes()
{
    return BasePointerRange(m_start, m_end);
}

template< typename T >
ALWAYS_INLINE const BasePointerRange PointerRange<T>::bytes() const
{
    return BasePointerRange(m_start, m_end);
}

template< typename T >
ALWAYS_INLINE PointerRange<T>::operator bool() const
{
    return !empty();
}

template< typename T >
ALWAYS_INLINE void PointerRange<T>::reset()
{
    m_start = nullptr;
    m_end = nullptr;
}

template< typename T >
ALWAYS_INLINE bool PointerRange<T>::containsRange(BasePointerRange other) const
{
    return other.empty() || (other.m_start >= (const uint8_t*)m_start && other.m_end <= (const uint8_t*)m_end);
}

template< typename T >
ALWAYS_INLINE bool PointerRange<T>::containsRange(PointerRange<T> other) const
{
    return other.empty() || (other.m_start >= m_start && other.m_end <= m_end);
}

template< typename T >
ALWAYS_INLINE bool PointerRange<T>::containsPointer(const void* ptr) const
{
    return (uint8_t*)ptr >= (uint8_t*)m_start && (uint8_t*)ptr < (uint8_t*)m_end;
}

template< typename T >
ALWAYS_INLINE T& PointerRange<T>::operator[](Index index)
{
    return m_start[index];
}

template< typename T >
ALWAYS_INLINE const T& PointerRange<T>::operator[](Index index) const
{
    return m_start[index];
}

//--

template< typename T >
ALWAYS_INLINE void PointerRange<T>::destroyElements()
{
    std::destroy_n(m_start, m_end - m_start);
}

template< typename T >
ALWAYS_INLINE void PointerRange<T>::constructElements()
{
    std::uninitialized_default_construct_n(m_start, m_end - m_start);
}

template< typename T >
ALWAYS_INLINE void PointerRange<T>::constructElementsFrom(const T& elementTemplate)
{
    std::uninitialized_fill_n(m_start, m_end - m_start, elementTemplate);
}

template< typename T >
ALWAYS_INLINE void PointerRange<T>::reverseElements()
{
    auto* cur = m_start;
    auto* end = m_end - 1;
    while (cur < end)
    {
        std::swap(*cur, *end);
        cur += 1;
        end -= 1;
    }
}

//--

template< typename T >
template< typename FK >
bool PointerRange<T>::contains(const FK& key) const
{
    auto* cur = m_start;
    while (cur < m_end)
        if (*cur++ == key)
            return true;

    return false;
}

//--

template< typename T >
template< typename FK >
bool PointerRange<T>::findFirst(const FK& key, Index& outFoundIndex) const
{
    auto* cur = m_start + outFoundIndex;
    while (++cur < m_end)
    {
        if (*cur == key)
        {
            outFoundIndex = cur - m_start;
            return true;
        }
    }

    return false;
}

template< typename T >
template< typename FK >
bool PointerRange<T>::findLast(const FK& key, Index& outFoundIndex) const
{
    auto* cur = m_start + outFoundIndex;
    while (--cur >= m_start)
    {
        if (*cur == key)
        {
            outFoundIndex = cur - m_start;
            return true;
        }
    }

    return false;
}

template<typename T>
template< typename FK >
Count PointerRange<T>::replaceAll(const FK& item, const T& itemTemplate)
{
    Count count = 0;
    auto* ptr = m_start;
    while (ptr < m_end)
    {
        if (*ptr == item)
        {
            *ptr = itemTemplate;
            count += 1;
        }
        ++ptr;
    }

    return count;
}

template<typename T>
template< typename FK >
bool PointerRange<T>::replaceFirst(const FK& item, T&& itemTemplate)
{
    auto* ptr = m_start;
    while (ptr < m_end)
    {
        if (*ptr == item)
        {
            *ptr = std::move(itemTemplate);
            return true;
        }
        ++ptr;
    }

    return false;
}

template<typename T>
template< typename FK >
bool PointerRange<T>::replaceFirst(const FK& item, const T& itemTemplate)
{
    auto* ptr = m_start;
    while (ptr < m_end)
    {
        if (*ptr == item)
        {
            *ptr = itemTemplate;
            return true;
        }
        ++ptr;
    }

    return false;

}

template<typename T>
template< typename FK >
bool PointerRange<T>::replaceLast(const FK& item, T&& itemTemplate)
{
    auto* ptr = m_end;
    while (--ptr >= m_start)
    {
        if (*ptr == item)
        {
            *ptr = std::move(itemTemplate);
            return true;
        }
        ++ptr;
    }

    return false;
}

template<typename T>
template< typename FK >
bool PointerRange<T>::replaceLast(const FK& item, const T& itemTemplate)
{
    auto* ptr = m_end;
    while (--ptr >= m_start)
    {
        if (*ptr == item)
        {
            *ptr = itemTemplate;
            return true;
        }
        ++ptr;
    }

    return false;
}

template< typename T >
void PointerRange<T>::sort()
{
    std::sort(begin(), end());
}

template< typename T >
template< typename Pred >
void PointerRange<T>::sort(const Pred& pred)
{
    std::sort(begin(), end(), pred);
}

template< typename T >
template< typename FK >
Index PointerRange<T>::lowerBound(const FK& key) const
{
    auto it = std::lower_bound(begin(), end(), key);
    return (it == m_array.end()) ? size() : std::distance(begin(), it);
}

template< typename T >
template< typename FK >
Index PointerRange<T>::upperBound(const FK& key) const
{
    auto it = std::upper_bound(begin(), end(), key);
    return (it == m_array.end()) ? size() : std::distance(begin(), it);
}

//--

template< typename T >
ALWAYS_INLINE ArrayIterator<T> PointerRange<T>::begin()
{
    return ArrayIterator<T>(m_start);
}

template< typename T >
ALWAYS_INLINE ArrayIterator<T> PointerRange<T>::end()
{
    return ArrayIterator<T>(m_end);
}

template< typename T >
ALWAYS_INLINE ConstArrayIterator<T> PointerRange<T>::begin() const
{
    return ConstArrayIterator<T>(m_start);
}

template< typename T >
ALWAYS_INLINE ConstArrayIterator<T> PointerRange<T>::end() const
{
    return ConstArrayIterator<T>(m_end);
}

//--

template< typename T >
template< typename K >
ALWAYS_INLINE PointerRange<K> PointerRange<T>::cast() const
{
    return PointerRange<K>((K*)m_start, (K*)m_end);
}

//--

END_BOOMER_NAMESPACE(base)
