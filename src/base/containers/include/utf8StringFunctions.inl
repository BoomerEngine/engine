/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string\utf8 #]
***/

#pragma once

/*
  BASED ON:

  Basic UTF-8 manipulation routines
  by Jeff Bezanson
  placed in the public domain Fall 2005
*/

BEGIN_BOOMER_NAMESPACE(base)

namespace utf8
{

    ///--

    INLINE CharIterator::CharIterator(const char* start, uint32_t length /*= UINT_MAX*/)
    {
        if (start)
        {
            if (length == UINT_MAX)
                length = strlen(start);
            m_pos = start;
            m_end = m_pos + length;
        }
    }

    INLINE CharIterator::CharIterator(const char* start, const char* end)
        : m_pos(start)
        , m_end(end)
    {
    }
            
    INLINE CharIterator::CharIterator(StringView txt)
        : m_pos(txt.data())
        , m_end(txt.data() + txt.length())
    {}

    INLINE CharIterator::operator bool() const
    {
        return ValidChar(m_pos, m_end);
    }

    INLINE uint32_t CharIterator::operator*() const
    {
        return GetChar(m_pos, m_end);
    }

    INLINE void CharIterator::operator++()
    {
        NextChar(m_pos, m_end);
    }

    INLINE void CharIterator::operator++(int)
    {
        NextChar(m_pos, m_end);
    }

    ///--

} // utf8

END_BOOMER_NAMESPACE(base)