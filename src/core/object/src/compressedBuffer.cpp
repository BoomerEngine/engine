/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: buffer #]
***/

#include "build.h"
#include "compressedBuffer.h"

BEGIN_BOOMER_NAMESPACE()

//--

CompressedBufer::CompressedBufer()
{
}

CompressedBufer::~CompressedBufer()
{
}

CompressedBufer::CompressedBufer(const CompressedBufer& other)
    : m_size(other.m_size)
    , m_crc(other.m_crc)
    , m_compression(other.m_compression)
    , m_data(other.m_data)
{}

CompressedBufer::CompressedBufer(CompressedBufer&& other)
    : m_data(std::move(other.m_data))
{
    m_size = other.m_size;
    m_crc = other.m_crc;
    m_compression = other.m_compression;

    other.m_size = 0;
    other.m_crc = 0;
    other.m_compression = CompressionType::Uncompressed;
}

CompressedBufer& CompressedBufer::operator=(const CompressedBufer& other)
{
    if (this != &other)
    {
        m_size = other.m_size;
        m_crc = other.m_crc;
        m_compression = other.m_compression;
        m_data = other.m_data;
    }

    return *this;
}

CompressedBufer& CompressedBufer::operator=(CompressedBufer&& other)
{
    if (this != &other)
    {
        m_data = std::move(other.m_data);

        m_size = other.m_size;
        m_crc = other.m_crc;
        m_compression = other.m_compression;
        other.m_size = 0;
        other.m_crc = 0;
        other.m_compression = CompressionType::Uncompressed;
    }

    return *this;
}

bool CompressedBufer::operator==(const CompressedBufer& other) const
{
    return m_crc == other.m_crc && m_size == other.m_size;
}

bool CompressedBufer::operator!=(const CompressedBufer& other) const
{
    return !operator==(other);
}

void CompressedBufer::reset()
{
    m_data.reset();
    m_size = 0;
    m_crc = 0;
    m_compression = CompressionType::Uncompressed;
}

void CompressedBufer::extract(Buffer& outCompressedData, CompressionType& outCompressiontType) const
{
    outCompressedData = m_data;
    outCompressiontType = m_compression;
}

void CompressedBufer::bindCompressedData(Buffer compressedData, CompressionType compression, uint64_t uncompressedSize, uint64_t uncompressedCRC)
{
    m_data = compressedData;
    m_size = uncompressedSize;
    m_crc = uncompressedCRC;
    m_compression = compression;
}

void CompressedBufer::bind(Buffer uncompressedData, CompressionType compression)
{
    // clear content
    reset();

    // bind only if we have valid data
    if (uncompressedData)
    {
        // calculate hash of the source data, this is the buffer "key"
        CRC64 crc;
        crc.append(uncompressedData.data(), uncompressedData.size());

        // compress the buffer
        Buffer compressedData;
        if (uncompressedData.size() < 512)
        {
            // don't compress small buffer
            compression = CompressionType::Uncompressed;
            compressedData = uncompressedData;
        }
        else if (compression != CompressionType::Uncompressed)
        {
            compressedData = Compress(compression, uncompressedData, POOL_MEM_BUFFER);

            // check that compression yield some result that is worth the CPU time down the line...
            const auto compressionCutoff = (uncompressedData.size() * 9) / 10; // 90%
            if (compressedData.size() > compressionCutoff)
            {
                compressedData = uncompressedData;
                compression = CompressionType::Uncompressed;
            }
        }
        else
        {
            // use uncompressed data
            compressedData = uncompressedData;
        }

        // initialize
        m_crc = crc;
        m_size = uncompressedData.size();
        m_compression = compression;
        m_data = compressedData;
    }
}

bool CompressedBufer::decompressInto(uint64_t offset, void* targetMemory, uint64_t targetSize) const
{
    DEBUG_CHECK_RETURN_EX_V(m_data, "Decompress called on invalid buffer, usually that's a bad sign", false);
    DEBUG_CHECK_RETURN_EX_V(offset <= size(), "Invalid offset", false);
    DEBUG_CHECK_RETURN_EX_V(offset + targetSize <= size(), "Invalid memory range", false);
    DEBUG_CHECK_RETURN_EX_V(m_compression == CompressionType::Uncompressed || offset == 0, "Decompress into with non zero offset called on a compressed buffer, this is VERY bad for perf", false);

    if (const auto data = decompress(POOL_TEMP, 16))
    {
        memcpy(targetMemory, data.data() + offset, targetSize);
        return true;
    }

    return false;
}

Buffer CompressedBufer::decompress(PoolTag tag, uint32_t alignment) const
{
    if (m_compression == CompressionType::Uncompressed)
        return m_data;

    PC_SCOPE_LVL0(DecompressBuffer);

    auto data = Buffer::Create(tag, m_size, alignment);
    DEBUG_CHECK_RETURN_EX_V(data, "Failed to create decompressed file buffer", Buffer());

    const auto ret = Decompress(m_compression, m_data.data(), m_data.size(), data.data(), m_size);
    DEBUG_CHECK_RETURN_EX_V(ret, "Failed to decompress file buffer", Buffer());

    return data;
}

//--

END_BOOMER_NAMESPACE()
