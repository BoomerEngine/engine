/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: event #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

ALWAYS_INLINE GlobalEventKey::GlobalEventKey(EGlobalKeyAutoInit)
{
    m_key = MakeUniqueEventKey().rawValue();
}

ALWAYS_INLINE GlobalEventKey::GlobalEventKey(GlobalEventKey&& other)
{
    m_key = other.m_key;
    other.m_key = 0;

#ifdef GLOBAL_EVENTS_DEBUG_INFO
    m_debugInfo = std::move(m_debugInfo);
#endif
}

ALWAYS_INLINE GlobalEventKey& GlobalEventKey::operator=(GlobalEventKey&& other)
{
    if (this != &other)
    {
        m_key = other.m_key;
        other.m_key = 0;

#ifdef GLOBAL_EVENTS_DEBUG_INFO
        m_debugInfo = std::move(m_debugInfo);
#endif
    }

    return *this;
}

ALWAYS_INLINE GlobalEventKey::operator bool() const
{
    return m_key != 0;
}

ALWAYS_INLINE bool GlobalEventKey::valid() const
{
    return  m_key != 0;
}

ALWAYS_INLINE GlobalEventKeyType GlobalEventKey::rawValue() const
{
    return m_key;
}

ALWAYS_INLINE bool GlobalEventKey::operator==(const GlobalEventKey& other) const
{
    return m_key == other.m_key;
}

ALWAYS_INLINE bool GlobalEventKey::operator!=(const GlobalEventKey& other) const
{
    return m_key == other.m_key;
}

ALWAYS_INLINE bool GlobalEventKey::operator<(const GlobalEventKey& other) const
{
    return m_key < other.m_key;
}

ALWAYS_INLINE uint32_t GlobalEventKey::CalcHash(const GlobalEventKey& key)
{
    return CRC32() << key.m_key;
}

//--

END_BOOMER_NAMESPACE()
