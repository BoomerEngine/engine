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

INLINE ResourceID::ResourceID(ResourceID&& other)
    : m_guid(std::move(other.m_guid))
{
    other.m_guid = GUID();
}

INLINE ResourceID& ResourceID::operator=(ResourceID&& other)
{
    if (this != &other)
    {
        m_guid = std::move(other.m_guid);
        other.m_guid = GUID();
    }
    return *this;
}

INLINE bool ResourceID::operator==(const ResourceID& other) const
{
    return (m_guid == other.m_guid);
}

INLINE bool ResourceID::operator!=(const ResourceID& other) const
{
    return !operator==(other);
}

INLINE bool ResourceID::empty() const
{
    return m_guid.empty();
}

INLINE ResourceID::operator bool() const
{
    return !empty();
}

INLINE uint32_t ResourceID::CalcHash(const ResourceID& key)
{
    return GUID::CalcHash(key.m_guid);
}

//--

END_BOOMER_NAMESPACE()
