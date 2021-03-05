/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "core/system/include/guid.h"

BEGIN_BOOMER_NAMESPACE()

/// resource ID
class CORE_RESOURCE_API ResourceID
{
public:
    INLINE ResourceID() {};
    INLINE ResourceID(std::nullptr_t) {};
    INLINE ResourceID(const GUID& id) : m_guid(id) {};
    INLINE ResourceID(const ResourceID& other) = default;
    INLINE ResourceID(ResourceID&& other);
    INLINE ResourceID& operator=(const ResourceID& other) = default;
    INLINE ResourceID& operator=(ResourceID&& other);

    INLINE bool operator==(const ResourceID& other) const;
    INLINE bool operator!=(const ResourceID& other) const;

    //--

    INLINE const GUID& guid() const { return m_guid; }

    INLINE bool empty() const;
    INLINE operator bool() const;

    INLINE static uint32_t CalcHash(const ResourceID& key);

    //--

    // prints the GUID
    void print(IFormatStream& f) const;

    //--

    // parse guid
    static bool Parse(StringView path, ResourceID& outPath);

    //--

    // empty key
    static const ResourceID& EMPTY();

    // create new resource ID
    static ResourceID Create();

    //--

private:
    GUID m_guid;
};

END_BOOMER_NAMESPACE()

#include "id.inl"
