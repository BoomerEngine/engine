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

#include "core/object/include/asyncBuffer.h"
#include "core/object/include/compressedBuffer.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE()

#pragma optimize("",off)

namespace prv
{

    void WriteBufferBinary(TypeSerializationContext& typeContext, SerializationWriter& stream, const void* data, const void* defaultData)
    {
        const auto& buffer = *(const Buffer*)data;

        const auto crc = CRC64().append(buffer.data(), buffer.size());
        stream.writeBuffer(buffer, CompressionType::Uncompressed, buffer.size(), crc);
    }

    void ReadBufferBinary(TypeSerializationContext& typeContext, SerializationReader& stream, void* data)
    {
        auto& buffer = *(Buffer*)data;
        buffer = stream.readUncompressedBuffer();
    }

    void WriteBufferXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData)
    {
        const auto& buffer = *(const Buffer*)data;
        node.writeBuffer(buffer);
    }

    void ReadBufferXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data)
    {
        auto& buffer = *(Buffer*)data;
        buffer = node.valueBufferBase64();
    }

} // prv

RTTI_BEGIN_CUSTOM_TYPE(Buffer);
    RTTI_BIND_NATIVE_CTOR_DTOR(Buffer);
    RTTI_BIND_NATIVE_COPY(Buffer);
    RTTI_BIND_NATIVE_COMPARE(Buffer);
    RTTI_BIND_NATIVE_PRINT(Buffer);
    RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteBufferBinary, &prv::ReadBufferBinary);
    RTTI_BIND_CUSTOM_XML_SERIALIZATION(&prv::WriteBufferXML, &prv::ReadBufferXML);
RTTI_END_TYPE();

//--

namespace prv
{

    void WriteAsyncBufferBinary(TypeSerializationContext& typeContext, SerializationWriter& stream, const void* data, const void* defaultData)
    {
        const auto& asyncBuffer = *(const AsyncFileBuffer*)data;

        if (auto loader = asyncBuffer.loader())
            stream.writeAsyncBuffer(loader);
        else
            stream.writeBuffer(Buffer(), CompressionType::Uncompressed, 0, 0);
    }

    void ReadAsyncBufferBinary(TypeSerializationContext& typeContext, SerializationReader& stream, void* data)
    {
        auto& asyncBuffer = *(AsyncFileBuffer*)data;

        const auto loader = stream.readAsyncBuffer();
        asyncBuffer = AsyncFileBuffer(loader);
    }

    void WriteAsyncBufferXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData)
    {
        const auto& asyncBuffer = *(const AsyncFileBuffer*)data;
        const auto content = asyncBuffer.load();
        node.writeBuffer(content);
    }

    void ReadAsyncBufferXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data)
    {
        auto& asyncBuffer = *(AsyncFileBuffer*)data;

        const auto buffer = node.valueBufferBase64();
        asyncBuffer.bind(buffer);
    }

} // prv

//--

RTTI_BEGIN_CUSTOM_TYPE(AsyncFileBuffer);
    RTTI_BIND_NATIVE_CTOR_DTOR(AsyncFileBuffer);
    RTTI_BIND_NATIVE_COMPARE(AsyncFileBuffer);
    RTTI_BIND_NATIVE_COPY(AsyncFileBuffer);
    RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteAsyncBufferBinary, &prv::ReadAsyncBufferBinary);
    RTTI_BIND_CUSTOM_XML_SERIALIZATION(&prv::WriteAsyncBufferXML, &prv::ReadAsyncBufferXML);
RTTI_END_TYPE();

//--

namespace prv
{

    void WriteCompressedBuferBinary(TypeSerializationContext& typeContext, SerializationWriter& stream, const void* data, const void* defaultData)
    {
        const auto& compressedBuffer = *(const CompressedBufer*)data;

        Buffer compressedData;
        CompressionType compressionType = CompressionType::Uncompressed;
        compressedBuffer.extract(compressedData, compressionType);

        stream.writeBuffer(compressedData, compressionType, compressedBuffer.size(), compressedBuffer.crc());
    }

    void ReadCompressedBuferBinary(TypeSerializationContext& typeContext, SerializationReader& stream, void* data)
    {
        uint64_t uncompressedSize = 0;
        uint64_t uncompressedCRC = 0;
        CompressionType compressionType;
        const auto compressedData = stream.readCompressedBuffer(compressionType, uncompressedSize, uncompressedCRC);

        auto& compressedBuffer = *(CompressedBufer*)data;
        compressedBuffer.bindCompressedData(compressedData, compressionType, uncompressedSize, uncompressedCRC);
    }

    void WriteCompressedBuferXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData)
    {
        const auto& compressedBuffer = *(const CompressedBufer*)data;
        const auto content = compressedBuffer.decompress();
        node.writeBuffer(content);
    }

    void ReadCompressedBuferXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data)
    {
        const auto buffer = node.valueBufferBase64();
        const auto crc = CRC64().append(buffer.data(), buffer.size());

        auto& compressedBuffer = *(CompressedBufer*)data;
        compressedBuffer.bindCompressedData(buffer, CompressionType::Uncompressed, buffer.size(), crc);
    }

} // prv

RTTI_BEGIN_CUSTOM_TYPE(CompressedBufer);
    RTTI_BIND_NATIVE_CTOR_DTOR(CompressedBufer);
    RTTI_BIND_NATIVE_COMPARE(CompressedBufer);
    RTTI_BIND_NATIVE_COPY(CompressedBufer);
    RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteCompressedBuferBinary, &prv::ReadCompressedBuferBinary);
    RTTI_BIND_CUSTOM_XML_SERIALIZATION(&prv::WriteCompressedBuferXML, &prv::ReadCompressedBuferXML);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE()

