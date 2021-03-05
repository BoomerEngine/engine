/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "id.h"

BEGIN_BOOMER_NAMESPACE()

RTTI_BEGIN_CUSTOM_TYPE(ResourceID);
    RTTI_BIND_NATIVE_COPY(ResourceID);
    RTTI_BIND_NATIVE_COMPARE(ResourceID);
    RTTI_BIND_NATIVE_CTOR_DTOR(ResourceID);
    RTTI_BIND_NATIVE_PRINT_PARSE(ResourceID);
RTTI_END_TYPE();

void ResourceID::print(IFormatStream& f) const
{
    m_guid.print(f);
}

bool ResourceID::Parse(StringView path, ResourceID& outPath)
{
    GUID id;
    if (!GUID::Parse(path.data(), path.length(), id))
        return false;

    outPath = ResourceID(id);
    return true;
}

const ResourceID& ResourceID::EMPTY()
{
    static ResourceID theEmptyID;
    return theEmptyID;
}

ResourceID ResourceID::Create()
{
    const auto id = GUID::Create();
    return ResourceID(id);
}

END_BOOMER_NAMESPACE()
