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
#include "serializationReader.h"
#include "asyncBuffer.h"

BEGIN_BOOMER_NAMESPACE()

#pragma optimize("",off)

//--

ISerializationBufferFactory::~ISerializationBufferFactory()
{}

//--

SerializationResolvedResourceReference::SerializationResolvedResourceReference()
{}

//--

SerializationResolvedReferences::SerializationResolvedReferences()
{}

//--

SerializationReader::SerializationReader(const SerializationResolvedReferences& refs, const void* data, uint64_t size, bool safeLayout, uint32_t version)
    : m_refs(refs)
    , m_protectedStream(safeLayout)
    , m_version(version)
{
    m_base = (const uint8_t*)data;
    m_cur = m_base;
    m_end = m_base + size;
}

SerializationReader::~SerializationReader()
{}

Buffer SerializationReader::readBufferData(uint64_t size, bool makeCopy)
{
    ASSERT_EX(m_cur + size < m_end, "Read past buffer end");

    const auto* start = m_cur;
    m_cur += size;

    if (makeCopy)
        return Buffer::Create(POOL_SERIALIZATION, size, 16, start);
    else
        return Buffer::CreateExternal(POOL_SERIALIZATION, size, (void*)start);
}

void SerializationReader::readBufferInfo(SerializationBufferInfo& outInfo)
{
    outInfo.uncompressedSize = readCompressedNumber();
    outInfo.uncompressedCRC = readCompressedNumber();
    outInfo.compressedSize = readCompressedNumber();

    outInfo.externalBuffer = (outInfo.compressedSize & 1) != 0;
    outInfo.compressedSize >>= 1;

    outInfo.compressionType = (CompressionType)(outInfo.compressedSize & 15);
    outInfo.compressedSize >>= 4;
}

AsyncFileBufferLoaderPtr SerializationReader::readAsyncBuffer()
{
    checkOp(SerializationOpcode::DataInlineBuffer);

    SerializationBufferInfo state;
    readBufferInfo(state);

    if (!state.uncompressedSize)
        return nullptr;

    if (state.externalBuffer)
    {
        DEBUG_CHECK_RETURN_EX_V(m_refs.bufferFactory, "No async buffer factory", nullptr);
        return m_refs.bufferFactory->createAsyncBufferLoader(state.uncompressedCRC);
    }
    else if (state.compressionType != CompressionType::Uncompressed)
    {
        const auto compressedData = readBufferData(state.compressedSize, true);
        return IAsyncFileBufferLoader::CreateResidentBufferFromAlreadyCompressedData(compressedData, state.uncompressedSize, state.uncompressedCRC, state.compressionType);
    }
    else
    {
        const auto uncompressedData = readBufferData(state.uncompressedSize, true);
        return IAsyncFileBufferLoader::CreateResidentBufferFromUncompressedData(uncompressedData, state.compressionType, state.uncompressedCRC);
    }
}

Buffer SerializationReader::readCompressedBuffer(CompressionType& outCompression, uint64_t& outSize, uint64_t& outCRC)
{
    checkOp(SerializationOpcode::DataInlineBuffer);

    SerializationBufferInfo state;
    readBufferInfo(state);

    if (!state.compressedSize)
        return nullptr;

    outCompression = state.compressionType;
    outSize = state.uncompressedSize;
    outCRC = state.uncompressedCRC;

    if (state.externalBuffer)
    {
        DEBUG_CHECK_RETURN_EX_V(m_refs.bufferFactory, "No async buffer factory", nullptr);

        auto data = m_refs.bufferFactory->loadAsyncBufferContent(state.uncompressedCRC);
        DEBUG_CHECK_RETURN_EX_V(data, "Unable to restore async buffer", nullptr);

        return data;
    }
    else
    {
        return readBufferData(state.compressedSize, true);
    }
}

Buffer SerializationReader::readUncompressedBuffer()
{
    checkOp(SerializationOpcode::DataInlineBuffer);

    SerializationBufferInfo state;
    readBufferInfo(state);

    if (!state.uncompressedSize)
        return nullptr;

    Buffer compressedData;

    if (state.externalBuffer)
    {
        DEBUG_CHECK_RETURN_EX_V(m_refs.bufferFactory, "No async buffer factory", nullptr);
        compressedData = m_refs.bufferFactory->loadAsyncBufferContent(state.uncompressedCRC);
        DEBUG_CHECK_RETURN_EX_V(compressedData, "Unable to restore async buffer", nullptr);

        if (state.compressionType == CompressionType::Uncompressed)
            return compressedData;
    }
    else
    {
        if (state.compressionType == CompressionType::Uncompressed)
            return readBufferData(state.compressedSize, true);

        compressedData = readBufferData(state.compressedSize, false); // no copy needed
    }
    
    auto data = Buffer::Create(POOL_SERIALIZATION, state.uncompressedSize, 16);
    DEBUG_CHECK_RETURN_EX_V(data, "Failed to create decompressed file buffer", Buffer());

    const auto ret = Decompress(state.compressionType, compressedData.data(), compressedData.size(), data.data(), state.uncompressedSize);
    DEBUG_CHECK_RETURN_EX_V(ret, "Failed to decompress file buffer", Buffer());

    return data;
}

void SerializationReader::discardSkipBlock()
{
    ASSERT_EX(m_protectedStream, "Trying to skip blocks in unprotected stream. This should have been found by validator.");
            
    checkOp(SerializationOpcode::SkipHeader);

    uint32_t skipLevel = 1;
    while (skipLevel && m_cur < m_end)
    {
        const auto op = (SerializationOpcode) *m_cur++;
        switch (op)
        {
            case SerializationOpcode::SkipHeader:
            {
                skipLevel += 1;
                break;
            }

            case SerializationOpcode::SkipLabel:
            {
                skipLevel -= 1;
                break;
            }

            case SerializationOpcode::Array:
            case SerializationOpcode::Compound:
            case SerializationOpcode::Property:
            case SerializationOpcode::DataTypeRef:
            case SerializationOpcode::DataName:
            case SerializationOpcode::DataObjectPointer:
            case SerializationOpcode::DataResourceRef:
            {
                readCompressedNumber();
                break;
            }

            case SerializationOpcode::DataRaw:
            {
                auto size = readCompressedNumber();
                ASSERT_EX(m_cur + size <= m_end, "Data size larger read region. This should be caught by validator.");
                m_cur += size;
                break;
            }

            case SerializationOpcode::DataInlineBuffer:
            {
                SerializationBufferInfo state;
                readBufferInfo(state);

                if (!state.externalBuffer)
                {
                    ASSERT_EX(m_cur + state.compressedSize <= m_end, "Data size larger read region. This should be caught by validator.");
                    m_cur += state.compressedSize;
                }
                break;
            }

            case SerializationOpcode::DataAsyncFileBuffer:
            {
                break;
            }

            case SerializationOpcode::CompoundEnd:
            case SerializationOpcode::ArrayEnd:
                break;

            case SerializationOpcode::Nop:
            default:
                ASSERT(!"Unknown opcode found when skipping. This should have been found by validator.");
        }

    }

    ASSERT_EX(skipLevel == 0, "End of stream reached before skip target was found. This should have been found by validator.");
}

//--

END_BOOMER_NAMESPACE()
