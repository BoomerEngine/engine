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
#include "streamOpcodes.h"
#include "streamOpcodeBinarizer.h"
#include "base/io/include/ioFileHandle.h"

namespace base
{
    namespace stream
    {
        //--

        OpcodeMappedReferences::OpcodeMappedReferences()
        {
            mappedNames.reserve(64);
            mappedTypes.reserve(64);
            mappedPointers.reserve(64);
            mappedProperties.reserve(64);
            mappedResources.reserve(64);
        }

        //--

        OpcodeFileWriter::OpcodeFileWriter(io::IWriteFileHandle* outputFile)
            : m_outputFile(outputFile)
        {}

        OpcodeFileWriter::~OpcodeFileWriter()
        {
            flush();
        }

        void OpcodeFileWriter::flush()
        {
            if (m_cacheBufferSize > 0)
            {
                m_crc.append(m_cacheBuffer, m_cacheBufferSize);
                m_outputFile->writeSync(m_cacheBuffer, m_cacheBufferSize);
                m_cacheBufferSize = 0;
            }
        }

        void OpcodeFileWriter::writeToFile(const void* data, uint64_t size)
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

        uint8_t MeasureIndexSize(uint64_t index)
        {
            uint8_t tempBuffer[16];
            return WriteCompressedUint64(tempBuffer, index);
        }

        void WriteIndex(OpcodeFileWriter& writer, uint64_t index)
        {
            uint8_t tempBuffer[16];
            auto size = WriteCompressedUint64(tempBuffer, index);
            writer.writeToFile(tempBuffer, size);
        }

        uint32_t ResolvePropertyIndex(const OpcodeMappedReferences& mappedReferences, const rtti::Property* prop)
        {
            uint32_t index = 0;
            if (!mappedReferences.mappedProperties.find(prop, index))
            {
                ASSERT(!"Property not mapped");
            }

            return index;
        }

        uint32_t ResolveTypeIndex(const OpcodeMappedReferences& mappedReferences, Type type)
        {
            uint32_t index = 0;
            if (!mappedReferences.mappedTypes.find(type, index))
            {
                ASSERT(!"Property not mapped");
            }

            return index;
        }

        uint32_t ResolveNameIndex(const OpcodeMappedReferences& mappedReferences, StringID name)
        {
            uint32_t index = 0;
            if (!mappedReferences.mappedNames.find(name, index))
            {
                ASSERT(!"Name not mapped");
            }

            return index;
        }

        uint32_t ResolveObjectIndex(const OpcodeMappedReferences& mappedReferences, const IObject* obj)
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

        uint32_t ResolveResourceIndex(const OpcodeMappedReferences& mappedReferences, const StringBuf& path, ClassType type)
        {
            if (!path || !type)
                return 0;

            OpcodeWriterResourceReference key;
            key.resourcePath = path;
            key.resourceType = type;

            uint32_t index = 0;
            if (!mappedReferences.mappedResources.find(key, index))
            {
                ASSERT(!"Resource not mapped");
            }

            ASSERT_EX(index != 0, "Valid resource cannot be mapped to zero");
            return index;
        }

        void WriteOpcodes(bool protectedStream, const OpcodeStream& stream, const OpcodeMappedReferences& mappedReferences, OpcodeFileWriter& writer)
        {
            for (auto it = stream.opcodes(); it; ++it)
            {
                if (protectedStream)
                    writer.writeToFile(&it->op, 1);

                switch (it->op)
                {
                case StreamOpcode::Compound:
                {
                    const auto* op = (const StreamOpCompound*)(*it);
                    WriteIndex(writer, op->numProperties);
                    break;
                }

                case StreamOpcode::CompoundEnd:
                    break;

                case StreamOpcode::Array:
                {
                    const auto* op = (const StreamOpArray*)(*it);
                    WriteIndex(writer, op->count);
                    break;
                }

                case StreamOpcode::ArrayEnd:
                    break;

                case StreamOpcode::SkipHeader:
                    break;

                case StreamOpcode::SkipLabel:
                    break;

                case StreamOpcode::Property:
                {
                    const auto* op = (const StreamOpProperty*)(*it);
                    WriteIndex(writer, ResolvePropertyIndex(mappedReferences, op->prop));
                    break;
                }

                case StreamOpcode::DataRaw:
                {
                    const auto* op = (const StreamOpDataRaw*)(*it);
                    const auto dataSize = op->dataSize();

                    if (protectedStream)
                        WriteIndex(writer, dataSize); // data size is needed for skipping

                    writer.writeToFile(op->data(), dataSize);
                    break;
                }

                case StreamOpcode::DataTypeRef:
                {
                    const auto* op = (const StreamOpDataTypeRef*)(*it);
                    WriteIndex(writer, ResolveTypeIndex(mappedReferences, op->type));
                    break;
                }

                case StreamOpcode::DataName:
                {
                    const auto* op = (const StreamOpDataName*)(*it);
                    WriteIndex(writer, ResolveNameIndex(mappedReferences, op->name));
                    break;
                }

                case StreamOpcode::DataObjectPointer:
                {
                    const auto* op = (const StreamOpDataObjectPointer*)(*it);
                    WriteIndex(writer, ResolveObjectIndex(mappedReferences, op->object));
                    break;
                }

                case StreamOpcode::DataResourceRef:
                {
                    const auto* op = (const StreamOpDataResourceRef*)(*it);
                    WriteIndex(writer, ResolveResourceIndex(mappedReferences, op->path, op->type));
                    break;
                }

                case StreamOpcode::DataInlineBuffer:
                {
                    const auto* op = (const StreamOpDataInlineBuffer*)(*it);
                    const auto dataSize = op->buffer.size();
                    WriteIndex(writer, dataSize);
                    writer.writeToFile(op->buffer.data(), dataSize);
                    break;
                }

                case StreamOpcode::DataAsyncBuffer:
                case StreamOpcode::Nop:
                    ASSERT(!"Invalid opcode");
                    break;

                default:
                    ASSERT(!"Unknown opcode");
                }
            }
        }
        
        //--

    } // stream
} // base
