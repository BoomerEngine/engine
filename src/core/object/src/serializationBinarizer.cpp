/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiProperty.h"

#include "serializationStream.h"
#include "serializationBinarizer.h"

#include "core/io/include/fileHandle.h"

BEGIN_BOOMER_NAMESPACE()

#pragma optimize("",off)

//--

SerializationMappedReferences::SerializationMappedReferences()
{
    mappedNames.reserve(64);
    mappedTypes.reserve(64);
    mappedPointers.reserve(64);
    mappedProperties.reserve(64);
    mappedResources.reserve(64);
}

//--

SerializationBinaryPacker::SerializationBinaryPacker(IWriteFileHandle* outputFile)
    : m_outputFile(outputFile)
{}

SerializationBinaryPacker::~SerializationBinaryPacker()
{
    flush();
}

void SerializationBinaryPacker::flush()
{
    if (m_cacheBufferSize > 0)
    {
        m_crc.append(m_cacheBuffer, m_cacheBufferSize);
        m_outputFile->writeSync(m_cacheBuffer, m_cacheBufferSize);
        m_cacheBufferSize = 0;
    }
}

void SerializationBinaryPacker::writeToFile(const void* data, uint64_t size)
{
    if (m_cacheBufferSize + size <= BUFFER_SIZE)
    {
        memcpy(m_cacheBuffer + m_cacheBufferSize, data, size);
        m_cacheBufferSize += size;
    }
    else
    {
        flush();

        if (size < (BUFFER_SIZE / 2))
        {
            memcpy(m_cacheBuffer + 0, data, size);
            m_cacheBufferSize = size;
        }
        else
        {
            m_crc.append(data, size);
            m_outputFile->writeSync(data, size);
        }
    }
}

//--

static uint8_t MeasureIndexSize(uint64_t index)
{
    uint8_t tempBuffer[16];
    return WriteCompressedUint64(tempBuffer, index);
}

static void WriteIndex(SerializationBinaryPacker& writer, uint64_t index)
{
    uint8_t tempBuffer[16];
    auto size = WriteCompressedUint64(tempBuffer, index);
    writer.writeToFile(tempBuffer, size);
}

static uint32_t ResolvePropertyIndex(const SerializationMappedReferences& mappedReferences, const Property* prop)
{
    uint32_t index = 0;
    if (!mappedReferences.mappedProperties.find(prop, index))
    {
        ASSERT(!"Property not mapped");
    }

    return index;
}

static uint32_t ResolveTypeIndex(const SerializationMappedReferences& mappedReferences, Type type)
{
    uint32_t index = 0;
    if (!mappedReferences.mappedTypes.find(type, index))
    {
        ASSERT(!"Property not mapped");
    }

    return index;
}

static uint32_t ResolveNameIndex(const SerializationMappedReferences& mappedReferences, StringID name)
{
    uint32_t index = 0;
    if (!mappedReferences.mappedNames.find(name, index))
    {
        ASSERT(!"Name not mapped");
    }

    return index;
}

static uint32_t ResolveObjectIndex(const SerializationMappedReferences& mappedReferences, const IObject* obj)
{
    if (!obj)
        return 0;

    uint32_t index = 0;
    if (!mappedReferences.mappedPointers.find(obj, index))
    {
        ASSERT(!"Object not mapped");
    }

    //ASSERT_EX(index != 0, "Valid opcode cannot be mapped to zero");
    return index;
}

static uint32_t ResolveResourceIndex(const SerializationMappedReferences& mappedReferences, const GUID& id, ClassType type)
{
    if (!id || !type)
        return 0;

    SerializationWriterResourceReference key;
    key.resourceID = id;
    key.resourceType = type;

    uint32_t index = 0;
    if (!mappedReferences.mappedResources.find(key, index))
    {
        ASSERT(!"Resource not mapped");
    }

    ASSERT_EX(index != 0, "Valid resource cannot be mapped to zero");
    return index;
}

static void WriteBufferInfo(SerializationBinaryPacker& writer, const SerializationBufferInfo& info)
{
    uint64_t compressedSize = info.compressedSize;
    compressedSize <<= 4;
    compressedSize |= (int)info.compressionType & 15;

    compressedSize <<= 1;
    compressedSize |= info.externalBuffer ? 1 : 0;

    WriteIndex(writer, info.uncompressedSize);
    WriteIndex(writer, info.uncompressedCRC);
    WriteIndex(writer, compressedSize);
}

void WriteOpcodes(bool protectedStream, const SerializationStream& stream, const SerializationMappedReferences& mappedReferences, SerializationBinaryPacker& writer)
{
    for (auto it = stream.opcodes(); it; ++it)
    {
        if (protectedStream)
            writer.writeToFile(&it->op, 1);

        switch (it->op)
        {
        case SerializationOpcode::Compound:
        {
            const auto* op = (const SerializationOpCompound*)(*it);
            WriteIndex(writer, op->numProperties);
            break;
        }

        case SerializationOpcode::CompoundEnd:
            break;

        case SerializationOpcode::Array:
        {
            const auto* op = (const SerializationOpArray*)(*it);
            WriteIndex(writer, op->count);
            break;
        }

        case SerializationOpcode::ArrayEnd:
            break;

        case SerializationOpcode::SkipHeader:
            break;

        case SerializationOpcode::SkipLabel:
            break;

        case SerializationOpcode::Property:
        {
            const auto* op = (const SerializationOpProperty*)(*it);
            WriteIndex(writer, ResolvePropertyIndex(mappedReferences, op->prop));
            break;
        }

        case SerializationOpcode::DataRaw:
        {
            const auto* op = (const SerializationOpDataRaw*)(*it);
            const auto dataSize = op->dataSize();

            if (protectedStream)
                WriteIndex(writer, dataSize); // data size is needed for skipping

            writer.writeToFile(op->data(), dataSize);
            break;
        }

        case SerializationOpcode::DataTypeRef:
        {
            const auto* op = (const SerializationOpDataTypeRef*)(*it);
            WriteIndex(writer, ResolveTypeIndex(mappedReferences, op->type));
            break;
        }

        case SerializationOpcode::DataName:
        {
            const auto* op = (const SerializationOpDataName*)(*it);
            WriteIndex(writer, ResolveNameIndex(mappedReferences, op->name));
            break;
        }

        case SerializationOpcode::DataObjectPointer:
        {
            const auto* op = (const SerializationOpDataObjectPointer*)(*it);
            WriteIndex(writer, ResolveObjectIndex(mappedReferences, op->object));
            break;
        }

        case SerializationOpcode::DataResourceRef:
        {
            const auto* op = (const SerializationOpDataResourceRef*)(*it);
            WriteIndex(writer, ResolveResourceIndex(mappedReferences, op->id, op->type));
            break;
        }

        case SerializationOpcode::DataInlineBuffer:
        {
            const auto* op = (const SerializationOpDataInlineBuffer*)(*it);

            SerializationBufferInfo info;
            info.externalBuffer = op->asyncLoader;
            info.compressedSize = op->data.size();
            info.compressionType = op->compressionType;
            info.uncompressedSize = op->uncompressedSize;
            info.uncompressedCRC = op->uncompressedCRC;
            WriteBufferInfo(writer, info);

            if (op->asyncLoader)
            {
                ASSERT(mappedReferences.mappedAsyncBuffers.contains(op->asyncLoader));
            }
            else
            {
                writer.writeToFile(op->data.data(), op->data.size());
            }

            break;
        }

        case SerializationOpcode::DataAsyncFileBuffer:
        case SerializationOpcode::Nop:
            ASSERT(!"Invalid opcode");
            break;

        default:
            ASSERT(!"Unknown opcode");
        }
    }
}
        
//--

END_BOOMER_NAMESPACE()
