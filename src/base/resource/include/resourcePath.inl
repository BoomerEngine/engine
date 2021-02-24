/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base::res)

//--

INLINE ResourcePath::ResourcePath(ResourcePath&& other)
    : m_string(std::move(other.m_string))
    , m_hash(other.m_hash)
{
    other.m_hash = 0;
}

INLINE ResourcePath& ResourcePath::operator=(ResourcePath&& other)
{
    if (this != &other)
    {
        m_string = std::move(other.m_string);
        m_hash = other.m_hash;
        other.m_hash = 0;
    }
    return *this;
}

INLINE bool ResourcePath::operator==(const ResourcePath& other) const
{
    return (m_hash == other.m_hash) && (m_string == other.m_string);
}

INLINE bool ResourcePath::operator!=(const ResourcePath& other) const
{
    return !operator==(other);
}

INLINE bool ResourcePath::empty() const
{
    return m_string.empty();
}

INLINE bool ResourcePath::valid() const
{
    return !empty();
}

INLINE ResourcePath::operator bool() const
{
    return !empty();
}

ALWAYS_INLINE const StringBuf& ResourcePath::str() const
{
    return m_string;
}

ALWAYS_INLINE uint64_t ResourcePath::hash() const
{
    return m_hash;
}

INLINE StringView ResourcePath::view() const
{
    return m_string;
}

INLINE uint32_t ResourcePath::CalcHash(const ResourcePath& key)
{
    return key.m_hash;
}

//--

END_BOOMER_NAMESPACE(base::res)
