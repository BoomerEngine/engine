/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: customTypes #]
***/

#include "build.h"

#include "core/object/include/streamOpcodeWriter.h"
#include "core/object/include/streamOpcodeReader.h"
#include "core/io/include/timestamp.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE_EX(io)

namespace prv
{

    void WriteBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeWriter& stream, const void* rawData, const void* defaultData)
    {
        const auto& data = *(const io::TimeStamp*)rawData;
        stream.writeTypedData<uint64_t>(data.value());
    }

    void ReadBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeReader& stream, void* rawData)
    {
        uint64_t value = 0;
        stream.readTypedData(value);

        auto& data = *(io::TimeStamp*)rawData;
        data = io::TimeStamp(value);
    }

    void WriteXML(rtti::TypeSerializationContext& typeContext, xml::Node& node, const void* rawData, const void* defaultData)
    {
        const auto& data = *(const io::TimeStamp*)rawData;
        const auto value = data.value();
        node.writeValue(TempString("{}", value));
    }

    void ReadXML(rtti::TypeSerializationContext& typeContext, const xml::Node& node, void* rawData)
    {
        uint64_t value = 0;
                
        if (node.value().match(value) == MatchResult::OK)
        {
            auto& data = *(io::TimeStamp*)rawData;
            data = io::TimeStamp(value);
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
