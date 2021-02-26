/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

INLINE GUID::GUID()
{
    static_assert(NUM_WORDS == 4, "Adapt for different number");
    m_words[0] = 0;
    m_words[1] = 0;
    m_words[2] = 0;
    m_words[3] = 0;
}

INLINE GUID::GUID(const GUID& other)
{
    static_assert(NUM_WORDS == 4, "Adapt for different number");
    m_words[0] = other.m_words[0];
    m_words[1] = other.m_words[1];
    m_words[2] = other.m_words[2];
    m_words[3] = other.m_words[3];
}

INLINE GUID::GUID(GUID&& other)
{
    static_assert(NUM_WORDS == 4, "Adapt for different number");
    m_words[0] = other.m_words[0];
    m_words[1] = other.m_words[1];
    m_words[2] = other.m_words[2];
    m_words[3] = other.m_words[3];
    other.m_words[0] = 0;
    other.m_words[1] = 0;
    other.m_words[2] = 0;
    other.m_words[3] = 0;
}

INLINE GUID& GUID::operator=(const GUID& other)
{
    static_assert(NUM_WORDS == 4, "Adapt for different number");
    m_words[0] = other.m_words[0];
    m_words[1] = other.m_words[1];
    m_words[2] = other.m_words[2];
    m_words[3] = other.m_words[3];
    return *this;
}

INLINE GUID& GUID::operator=(GUID&& other)
{
    static_assert(NUM_WORDS == 4, "Adapt for different number");
    m_words[0] = other.m_words[0];
    m_words[1] = other.m_words[1];
    m_words[2] = other.m_words[2];
    m_words[3] = other.m_words[3];
    return *this;
}

INLINE const uint32_t* GUID::data() const
{
    return m_words;
}

INLINE bool GUID::empty() const
{
    static_assert(NUM_WORDS == 4, "Adapt for different number");
    return m_words[0] == 0 && m_words[1] == 0 && m_words[2] == 0 && m_words[3] == 0;
}

INLINE GUID::operator bool() const
{
    return !empty();
}

INLINE bool GUID::operator==(const GUID& other) const
{
    static_assert(NUM_WORDS == 4, "Adapt for different number");
    return (m_words[0] == other.m_words[0]) && (m_words[1] == other.m_words[1]) && (m_words[2] == other.m_words[2]) && (m_words[3] == other.m_words[3]);
}

INLINE bool GUID::operator!=(const GUID& other) const
{
    return !operator==(other);
}

INLINE bool GUID::operator<(const GUID& other) const
{
    return memcmp(m_words, other.m_words, sizeof(m_words)) < 0;
}

//--
    
END_BOOMER_NAMESPACE()
