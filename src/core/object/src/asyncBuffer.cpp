/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: buffer #]
***/

#include "build.h"
#include "asyncBuffer.h"

#include "core/object/include/serializationWriter.h"
#include "core/object/include/serializationReader.h"

BEGIN_BOOMER_NAMESPACE()

//--

IAsyncFileBufferLoader::~IAsyncFileBufferLoader()
{
}

//--

// wrapper for resident buffer (in memory, not yet saved) that was initially uncompressed
class ResidentAsyncFileBuffer : public IAsyncFileBufferLoader
{
public:
    ResidentAsyncFileBuffer(CompressionType ct, Buffer uncompresedBuffer, uint64_t uncompressedCRC)
        : m_crc(uncompressedCRC)
        , m_intendedCompressionType(ct)
        , m_uncompresedBuffer(uncompresedBuffer)
    {
    }

    virtual uint64_t size() const override final
    {
        return m_uncompresedBuffer.size();
    }

    virtual uint64_t crc() const override final
    {
        return m_crc;
    }

    virtual bool resident() const override final
    {
        return true;
    }

    virtual CAN_YIELD bool extract(Buffer& outCompressedData, CompressionType& outCompressionType) const override final
    {
        if (!m_uncompresedBuffer)
        {
            outCompressedData = Buffer();
            outCompressionType = CompressionType::Uncompressed;
            return true;
        }

        auto lock = CreateLock(m_compressedBufferLock);

        if (!m_compressedBuffer)
        {
            if (m_compressedBuffer.size() > 512 && m_intendedCompressionType != CompressionType::Uncompressed)
            {
                const auto compressedData = Compress(m_intendedCompressionType, m_uncompresedBuffer, POOL_ASYNC_BUFFER);

                // check that compression yield some result that is worth the CPU time down the line...
                const auto compressionCutoff = (m_uncompresedBuffer.size() * 9) / 10; // 90%
                if (compressedData && compressedData.size() < compressionCutoff)
                {
                    m_compressedBufferCompressionType = m_intendedCompressionType;
                    m_compressedBuffer = compressedData;
                }
            }

            if (!m_compressedBuffer)
            {
                m_compressedBufferCompressionType = CompressionType::Uncompressed;
                m_compressedBuffer = m_uncompresedBuffer;
            }
        }

        outCompressionType = m_compressedBufferCompressionType;
        outCompressedData = m_compressedBuffer;
        return true;
    }

    virtual CAN_YIELD Buffer load(PoolTag tag, uint32_t alignment) const override final
    {
        return m_uncompresedBuffer;
    }

    virtual CAN_YIELD bool loadInto(uint64_t offset, void* targetMemory, uint64_t targetSize) const override final
    {
        DEBUG_CHECK_RETURN_EX_V(offset <= m_uncompresedBuffer.size(), "Invalid offset", false);
        DEBUG_CHECK_RETURN_EX_V(offset + targetSize <= m_uncompresedBuffer.size(), "Invalid memory range", false);
        memcpy(m_uncompresedBuffer.data() + offset, targetMemory, targetSize);
        return true;
    }

private:
    uint64_t m_crc = 0;
    CompressionType m_intendedCompressionType;
    Buffer m_uncompresedBuffer;

    SpinLock m_compressedBufferLock;
    mutable CompressionType m_compressedBufferCompressionType; // actual
    mutable Buffer m_compressedBuffer;
};

AsyncFileBufferLoaderPtr IAsyncFileBufferLoader::CreateResidentBufferFromUncompressedData(Buffer uncompressedData, CompressionType compression /*= CompressionType::Uncompressed*/, uint64_t knownCRC /*= 0*/)
{
    if (!uncompressedData)
        return nullptr;
    
    // calculate the data fingerprint
    const auto crc = knownCRC ? knownCRC : CRC64().append(uncompressedData.data(), uncompressedData.size());
    return RefNew<ResidentAsyncFileBuffer>(compression, uncompressedData, crc);
}

//--

// wrapper for resident buffer (in memory, not yet saved) that was initially compressed
class ResidentAsyncFileCompressedBuffer : public IAsyncFileBufferLoader
{
public:
    ResidentAsyncFileCompressedBuffer(CompressionType ct, Buffer compresedBuffer, uint64_t uncompressedCRC, uint64_t uncompressedSize)
        : m_crc(uncompressedCRC)
        , m_size(uncompressedSize)
        , m_compressionType(ct)
        , m_compressedBuffer(compresedBuffer)
    {
    }

    virtual uint64_t size() const override final
    {
        return m_size;
    }

    virtual uint64_t crc() const override final
    {
        return m_crc;
    }

    virtual bool resident() const override final
    {
        return true;
    }

    virtual CAN_YIELD bool extract(Buffer& outCompressedData, CompressionType& outCompressionType) const override final
    {
        outCompressedData = m_compressedBuffer;
        outCompressionType = m_compressionType;
        return true;
    }

    virtual CAN_YIELD Buffer load(PoolTag tag, uint32_t alignment) const override final
    {
        if (m_compressionType == CompressionType::Uncompressed)
            return m_compressedBuffer;

        auto lock = CreateLock(m_uncompressedBufferLock);

        auto data = Buffer::Create(tag, m_size, alignment);
        DEBUG_CHECK_RETURN_EX_V(data, "Failed to create decompressed file buffer", Buffer());

        const auto ret = Decompress(m_compressionType, m_compressedBuffer.data(), m_compressedBuffer.size(), data.data(), m_size);
        DEBUG_CHECK_RETURN_EX_V(ret, "Failed to decompress file buffer", Buffer());

        m_uncompressedBuffer = data;
        return data;
    }

    virtual CAN_YIELD bool loadInto(uint64_t offset, void* targetMemory, uint64_t targetSize) const override final
    {
        DEBUG_CHECK_RETURN_EX_V(offset <= size(), "Invalid offset", false);
        DEBUG_CHECK_RETURN_EX_V(offset + targetSize <= size(), "Invalid memory range", false);

        const auto data = load(POOL_ASYNC_BUFFER, 16);
        if (!data)
            return false;

        memcpy(data.data() + offset, targetMemory, targetSize);
        return true;
    }

private:
    uint64_t m_crc = 0;
    uint64_t m_size = 0;

    CompressionType m_compressionType;
    Buffer m_compressedBuffer;

    SpinLock m_uncompressedBufferLock;
    mutable Buffer m_uncompressedBuffer;
};

AsyncFileBufferLoaderPtr IAsyncFileBufferLoader::CreateResidentBufferFromAlreadyCompressedData(Buffer compressedData, uint64_t uncompressedSize, uint64_t uncompressedCRC, CompressionType ct)
{
    if (!compressedData)
        return nullptr;

    DEBUG_CHECK_RETURN_EX_V(uncompressedSize != 0, "Invalid size of uncompressed data", nullptr);
    DEBUG_CHECK_RETURN_EX_V(uncompressedCRC != 0, "Invalid CRC of uncompressed data", nullptr);

    return RefNew<ResidentAsyncFileCompressedBuffer>(ct, compressedData, uncompressedCRC, uncompressedSize);
}

//--

AsyncFileBuffer::AsyncFileBuffer()
{
}

AsyncFileBuffer::~AsyncFileBuffer()
{
}

AsyncFileBuffer::AsyncFileBuffer(const AsyncFileBuffer& other)
    : m_loader(other.m_loader)
{}

AsyncFileBuffer::AsyncFileBuffer(IAsyncFileBufferLoader* loader)
    : m_loader(AddRef(loader))
{}

AsyncFileBuffer::AsyncFileBuffer(AsyncFileBuffer&& other)
    : m_loader(std::move(other.m_loader))
{}

AsyncFileBuffer& AsyncFileBuffer::operator=(const AsyncFileBuffer& other)
{
    m_loader = other.m_loader;
    return *this;
}

AsyncFileBuffer& AsyncFileBuffer::operator=(AsyncFileBuffer&& other)
{
    m_loader = std::move(other.m_loader);
    return *this;
}

bool AsyncFileBuffer::operator==(const AsyncFileBuffer& other) const
{
    return m_loader == other.m_loader;
}

bool AsyncFileBuffer::operator!=(const AsyncFileBuffer& other) const
{
    return m_loader != other.m_loader;
}

void AsyncFileBuffer::reset()
{
    m_loader.reset();
}

void AsyncFileBuffer::bind(Buffer uncompressedData, CompressionType compression, PoolTag tag)
{
    m_loader = IAsyncFileBufferLoader::CreateResidentBufferFromUncompressedData(uncompressedData, compression);
}

CAN_YIELD Buffer AsyncFileBuffer::load(PoolTag tag, uint32_t alignment) const
{
    DEBUG_CHECK_RETURN_EX_V(m_loader, "Load called on invalid buffer, usually that's a bad sign", Buffer());
    return m_loader->load(tag, alignment);
}

CAN_YIELD bool AsyncFileBuffer::loadInto(uint64_t offset, void* targetMemory, uint64_t targetSize) const
{
    DEBUG_CHECK_RETURN_EX_V(m_loader, "Load called on invalid buffer, usually that's a bad sign", false);
    DEBUG_CHECK_RETURN_EX_V(offset <= m_loader->size(), "Invalid offset", false);
    DEBUG_CHECK_RETURN_EX_V(offset + targetSize <= m_loader->size(), "Invalid memory range", false);
    return m_loader->loadInto(offset, targetMemory, targetSize);
}

//--

END_BOOMER_NAMESPACE()
