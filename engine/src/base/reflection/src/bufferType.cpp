/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: customTypes #]
***/

#include "build.h"

#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/streamTextWriter.h"
#include "base/object/include/streamTextReader.h"

namespace base
{
    namespace prv
    {

        bool WriteBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryWriter& stream, const void* data, const void* defaultData)
        {
            const auto& buffer = *(const Buffer*)data;
            //const auto& defaultBuffer = *(Buffer*)defaultData; // TODO: delta compress ? 

            bool allowCompression = true;
            if (buffer && !stream.isNull())
            {
                uint64_t compressedSize = 0;
                void* compressedData = allowCompression ? mem::Compress(mem::CompressionType::LZ4HC, buffer.data(), buffer.size(), compressedSize) : nullptr;                
                if (compressedData)
                {
                    //DEBUG_CHECK(compressedSize <= buffer.size());
                    auto cutoffSize = (buffer.size() * 15) / 16; // we need to be beneficial for all the work
                    if (compressedSize < cutoffSize)
                    {
                        stream.writeValue((uint8_t)2);
                        stream.writeValue(buffer.size());
                        stream.writeValue(compressedSize);
                        stream.write(compressedData, compressedSize);
                    }
                    else
                    {
                        stream.writeValue((uint8_t)1);
                        stream.writeValue(buffer.size());
                        stream.write(buffer.data(), buffer.size());
                    }

                    MemFree(compressedData);
                }
                else
                {
                    //DEBUG_CHECK(!"Compression failed for some reason");
                    stream.writeValue((uint8_t)1);
                    stream.writeValue(buffer.size());
                    stream.write(buffer.data(), buffer.size());
                }
            }
            else
            {
                stream.writeValue((uint8_t)0);
            }

            return true;
        }

        bool ReadBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryReader& stream, void* data)
        {
            auto& buffer = *(Buffer*)data;

            uint8_t code = 0;
            stream.readValue(code);

            DEBUG_CHECK_EX(code <= 2, "Invalid buffer code");
            if (code > 2)
                return false;

            uint64_t rawSize = 0;
            stream.readValue(rawSize);
            buffer.init(POOL_TEMP, rawSize);

            if (code == 1)
            {
                stream.read(buffer.data(), rawSize);
            }
            else if (code == 2)
            {
                uint64_t compressedSize = 0;
                stream.readValue(compressedSize);

                if (compressedSize >= rawSize)
                    return false;

                auto compressedBuffer = Buffer::Create(POOL_TEMP, compressedSize);
                if (!compressedBuffer)
                    return false;

                stream.read(compressedBuffer.data(), compressedSize);
                if (stream.isError())
                    return false;

                if (!mem::Decompress(mem::CompressionType::LZ4HC, compressedBuffer.data(), compressedBuffer.size(), buffer.data(), buffer.size()))
                {
                    DEBUG_CHECK(!"Decompression failed for some reason");
                    return false;
                }
            }

            return true;
        }

        bool WriteText(const rtti::TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData)
        {
            auto& buffer = *(Buffer*)data;
            stream.writeValue(buffer);
            return true;
        }

        bool ReadText(const rtti::TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data)
        {
            auto& buffer = *(Buffer*)data;
            return stream.readValue(buffer);
        }

    } // prv

    RTTI_BEGIN_CUSTOM_TYPE(Buffer);
        RTTI_BIND_NATIVE_CTOR_DTOR(Buffer);
        RTTI_BIND_NATIVE_COPY(Buffer);
        RTTI_BIND_NATIVE_COMPARE(Buffer);
        RTTI_BIND_NATIVE_PRINT(Buffer);
        RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteBinary, &prv::ReadBinary);
        RTTI_BIND_CUSTOM_TEXT_SERIALIZATION(&prv::WriteText, &prv::ReadText);
    RTTI_END_TYPE();

} // base
