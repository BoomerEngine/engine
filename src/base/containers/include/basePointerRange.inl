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

ALWAYS_INLINE BasePointerRange::BasePointerRange(void* data, uint64_t length)
{
    if (data && length)
    {
        m_start = (uint8_t*)data;
        m_end = m_start + length;
    }
}

ALWAYS_INLINE BasePointerRange::BasePointerRange(void* data, void* dataEnd)
{
    if (data && ((uint8_t*)dataEnd > (uint8_t*)data))
    {
        m_start = (uint8_t*)data;
        m_end = (uint8_t*)dataEnd;
    }
}

ALWAYS_INLINE BasePointerRange::BasePointerRange(const void* data, uint64_t length)
{
    if (data && length)
    {
        m_start = (uint8_t*)data;
        m_end = m_start + length;
    }
}

ALWAYS_INLINE BasePointerRange::BasePointerRange(const void* data, const void* dataEnd)
{
    if (data && ((uint8_t*)dataEnd > (uint8_t*)data))
    {
        m_start = (uint8_t*)data;
        m_end = (uint8_t*)dataEnd;
    }
}

//--

ALWAYS_INLINE uint8_t* BasePointerRange::data()
{
    return m_start;
}

ALWAYS_INLINE const uint8_t* BasePointerRange::data() const
{
    return m_start;
}

ALWAYS_INLINE uint64_t BasePointerRange::dataSize() const
{
    return m_end - m_start;
}

ALWAYS_INLINE bool BasePointerRange::empty() const
{
    return m_start == m_end;
}

//--

ALWAYS_INLINE void BasePointerRange::reset()
{
    m_start = nullptr;
    m_end = nullptr;
}

ALWAYS_INLINE bool BasePointerRange::containsRange(BasePointerRange other) const
{
    return other.empty() || (other.m_start >= (const uint8_t*)m_start && other.m_end <= (const uint8_t*)m_end);
}

ALWAYS_INLINE bool BasePointerRange::containsPointer(const void* ptr) const
{
    return (uint8_t*)ptr >= (uint8_t*)m_start && (uint8_t*)ptr < (uint8_t*)m_end;
}

//--

ALWAYS_INLINE ArrayIterator<uint8_t> BasePointerRange::begin()
{
    return ArrayIterator<uint8_t>(m_start);
}

ALWAYS_INLINE ArrayIterator<uint8_t> BasePointerRange::end()
{
    return ArrayIterator<uint8_t>(m_end);
}

ALWAYS_INLINE ConstArrayIterator<uint8_t> BasePointerRange::begin() const
{
    return ConstArrayIterator<uint8_t>(m_start);
}

ALWAYS_INLINE ConstArrayIterator<uint8_t> BasePointerRange::end() const
{
    return ConstArrayIterator<uint8_t>(m_end);
}
    
//--

END_BOOMER_NAMESPACE(base)
