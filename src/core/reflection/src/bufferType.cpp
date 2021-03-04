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

BEGIN_BOOMER_NAMESPACE()

namespace prv
{

    void WriteBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& stream, const void* data, const void* defaultData)
    {
        const auto& buffer = *(const Buffer*)data;
        stream.writeTypedData<uint8_t>(0); // compression type - none, allows for binary compatibility with compressed buffers
        stream.writeBuffer(buffer);
    }

    void ReadBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& stream, void* data)
    {
        auto& buffer = *(Buffer*)data;

        uint8_t code = 0;
        stream.readTypedData(code);

        ASSERT_EX(code == 0, "Invalid buffer code");
        buffer = stream.readBuffer();

        /*auto compressedBuffer = Buffer::Create(POOL_TEMP, compressedSize);
        if (!compressedBuffer)
            return false;

        stream.read(compressedBuffer.data(), compressedSize);
        if (stream.isError())
            return false;

        if (!Decompress(CompressionType::LZ4HC, compressedBuffer.data(), compressedBuffer.size(), buffer.data(), buffer.size()))
        {
            DEBUG_CHECK(!"Decompression failed for some reason");
            return false;
        }*/
    }

} // prv

RTTI_BEGIN_CUSTOM_TYPE(Buffer);
    RTTI_BIND_NATIVE_CTOR_DTOR(Buffer);
    RTTI_BIND_NATIVE_COPY(Buffer);
    RTTI_BIND_NATIVE_COMPARE(Buffer);
    RTTI_BIND_NATIVE_PRINT(Buffer);
    RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteBinary, &prv::ReadBinary);
RTTI_END_TYPE();

END_BOOMER_NAMESPACE()

