/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: customTypes #]
***/

#include "build.h"

#include "core/object/include/serializationWriter.h"
#include "core/object/include/serializationReader.h"
#include "core/io/include/timestamp.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE_EX(io)

namespace prv
{

    void WriteBinary(TypeSerializationContext& typeContext, SerializationWriter& stream, const void* rawData, const void* defaultData)
    {
        const auto& data = *(const TimeStamp*)rawData;
        stream.writeTypedData<uint64_t>(data.value());
    }

    void ReadBinary(TypeSerializationContext& typeContext, SerializationReader& stream, void* rawData)
    {
        uint64_t value = 0;
        stream.readTypedData(value);

        auto& data = *(TimeStamp*)rawData;
        data = TimeStamp(value);
    }

    void WriteXML(TypeSerializationContext& typeContext, xml::Node& node, const void* rawData, const void* defaultData)
    {
        const auto& data = *(const TimeStamp*)rawData;
        const auto value = data.value();
        node.writeValue(TempString("{}", value));
    }

    void ReadXML(TypeSerializationContext& typeContext, const xml::Node& node, void* rawData)
    {
        uint64_t value = 0;
                
        if (node.value().match(value) == MatchResult::OK)
        {
            auto& data = *(TimeStamp*)rawData;
            data = TimeStamp(value);
        }
    }

} // prv

RTTI_BEGIN_CUSTOM_TYPE(TimeStamp);
    RTTI_BIND_NATIVE_CTOR_DTOR(TimeStamp);
    RTTI_BIND_NATIVE_COPY(TimeStamp);
    RTTI_BIND_NATIVE_COMPARE(TimeStamp);
    RTTI_BIND_NATIVE_PRINT(TimeStamp);
    RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteBinary, &prv::ReadBinary);
    RTTI_BIND_CUSTOM_XML_SERIALIZATION(&prv::WriteXML, &prv::ReadXML);
RTTI_END_TYPE();

END_BOOMER_NAMESPACE_EX(io)
