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

ALWAYS_INLINE IndexRange::IndexRange(Index first, Count count)
    : m_first(first)
    , m_count(count)
{}

ALWAYS_INLINE IndexRange::IndexRange(IndexRange&& other)
    : m_first(other.m_first)
    , m_count(other.m_count)
{
    other.m_first = 0;
    other.m_count = 0;
}

ALWAYS_INLINE IndexRange& IndexRange::operator=(IndexRange&& other)
{
    if (this != &other)
    {
        m_first = other.m_first;
        m_count = other.m_count;
        other.m_first = 0;
        other.m_count = 0;
    }

    return *this;
}

//--

ALWAYS_INLINE Count IndexRange::size() const
{
    return m_count;
}

ALWAYS_INLINE bool IndexRange::empty() const
{
    return m_count == 0;
}

ALWAYS_INLINE IndexRange::operator bool() const
{
    return m_count != 0;
}

//--

ALWAYS_INLINE Index IndexRange::first() const
{
    checkNotEmpty();
    return m_first;
}

ALWAYS_INLINE Index IndexRange::last() const
{
    checkNotEmpty();
    return m_first + m_count - 1;
}

ALWAYS_INLINE IndexIterator IndexRange::begin() const
{
    return IndexIterator(m_first);
}

ALWAYS_INLINE IndexIterator IndexRange::end() const
{
    return IndexIterator(m_first + m_count);
}

ALWAYS_INLINE bool IndexRange::contains(Index index) const
{
    return (index != -1) && (index >= m_first) && (index < m_first + m_count);
}

ALWAYS_INLINE bool IndexRange::contains(IndexRange range) const
{
    return range.empty() || (contains(range.first()) && contains(range.last()));
}

ALWAYS_INLINE ReversedIndexRange IndexRange::reversed() const
{
    return empty() ? ReversedIndexRange() : ReversedIndexRange(last(), m_count);
}

//--

ALWAYS_INLINE ReversedIndexRange::ReversedIndexRange(Index last, Count count)
    : m_last(last)
    , m_count(count)
{}

ALWAYS_INLINE ReversedIndexRange::ReversedIndexRange(ReversedIndexRange&& other)
    : m_last(other.m_last)
    , m_count(other.m_count)
{
    other.m_last = 0;
    other.m_count = 0;
}

ALWAYS_INLINE ReversedIndexRange& ReversedIndexRange::operator=(ReversedIndexRange&& other)
{
    if (this != &other)
    {
        m_last = other.m_last;
        m_count = other.m_count;
        other.m_last = 0;
        other.m_count = 0;
    }

    return *this;
}

ALWAYS_INLINE Count ReversedIndexRange::size() const
{
    return m_count;
}

ALWAYS_INLINE bool ReversedIndexRange::empty() const
{
    return m_count == 0;
}

ALWAYS_INLINE ReversedIndexRange::operator bool() const
{
    return m_count != 0;
}

//--

ALWAYS_INLINE Index ReversedIndexRange::first() const
{
    checkNotEmpty();
    return m_last - (m_count - 1);
}

ALWAYS_INLINE Index ReversedIndexRange::last() const
{
    checkNotEmpty();
    return m_last;
}

ALWAYS_INLINE bool ReversedIndexRange::contains(Index index) const
{
    return (index != -1) && (index > (m_last - m_count)) && (index <= m_last);
}

ALWAYS_INLINE bool ReversedIndexRange::contains(ReversedIndexRange range) const
{
    return range.empty() || (contains(range.first()) && contains(range.last()));
}

ALWAYS_INLINE IndexRange ReversedIndexRange::reversed() const
{
    return empty() ? IndexRange() : IndexRange(first(), m_count);
}

ALWAYS_INLINE ReversedIndexIterator ReversedIndexRange::begin() const
{
    return ReversedIndexIterator(m_last);
}

ALWAYS_INLINE ReversedIndexIterator ReversedIndexRange::end() const
{
    return ReversedIndexIterator(m_last - m_count);
}

//--

END_BOOMER_NAMESPACE(base)

