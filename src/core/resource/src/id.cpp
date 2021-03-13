/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "id.h"

#include "core/object/include/serializationWriter.h"
#include "core/object/include/serializationReader.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{

    void WriteResourceIDBinary(TypeSerializationContext& typeContext, SerializationWriter& stream, const void* data, const void* defaultData)
    {
        const auto& id = *(const ResourceID*)data;
        stream.writeTypedData<GUID>(id.guid());
    }

    void ReadResourceIDBinary(TypeSerializationContext& typeContext, SerializationReader& stream, void* data)
    {
        GUID guid;
        stream.readTypedData<GUID>(guid);

        auto& id = *(ResourceID*)data;
        id = guid;
    }

    void WriteResourceIDXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData)
    {
        const auto& id = *(const ResourceID*)data;
        if (id)
            node.writeValue(TempString("{}", id));
    }

    void ReadResourceIDXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data)
    {
        ResourceID parsed;
        if (auto txt = node.value())
        {
            if (!ResourceID::Parse(txt, parsed))
            {
                TRACE_ERROR("{}: unable to parse resource ID from '{}'", typeContext, txt);
            }
        }

        auto& id = *(ResourceID*)data;
        id = parsed;
    }

} // prv

RTTI_BEGIN_CUSTOM_TYPE(ResourceID);
    RTTI_BIND_NATIVE_COPY(ResourceID);
    RTTI_BIND_NATIVE_COMPARE(ResourceID);
    RTTI_BIND_NATIVE_CTOR_DTOR(ResourceID);
    RTTI_BIND_NATIVE_PRINT_PARSE(ResourceID);
    RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteResourceIDBinary, &prv::ReadResourceIDBinary);
    RTTI_BIND_CUSTOM_XML_SERIALIZATION(&prv::WriteResourceIDXML, &prv::ReadResourceIDXML);
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
