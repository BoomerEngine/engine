/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: buffer #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

// potentially compressed data buffer stored on disk, special care is taken that the recompression does not happen during load/save cycle
class CORE_OBJECT_API CompressedBufer
{
public:
    CompressedBufer();
    CompressedBufer(const CompressedBufer& other);
    CompressedBufer(CompressedBufer&& other);
    ~CompressedBufer();

    CompressedBufer& operator=(const CompressedBufer& other);
    CompressedBufer& operator=(CompressedBufer&& other);

    bool operator==(const CompressedBufer& other) const;
    bool operator!=(const CompressedBufer& other) const;

    //--

    // get UNCOMPRESSED size of the data in this buffer
    INLINE uint64_t size() const { return m_size; }

    // get the CRC of the data stored in the buffer (without loading it)
    // NOTE: the CRC can act as a hash in some other data structures
    INLINE uint64_t crc() const { return m_crc; }

    // returns true if buffer is empty (does not contain any data)
    INLINE bool empty() const { return 0 == m_size; }

    // check if buffer is valid
    INLINE operator bool() const { return 0 != m_size; }

    //--

    // clear data in the buffer
    void reset();

    // set new data for the data buffer, data will be optionally compressed
    // NOTE: the source buffer MUST NOT be modified after binding
    void bind(Buffer uncompressedData, CompressionType compression = CompressionType::Uncompressed);

    // bind already compressed data (no recompression will happen)
    void bindCompressedData(Buffer compressedData, CompressionType compression, uint64_t uncompressedSize, uint64_t uncompressedCRC);

    //--

    // decompress content of the buffer
    CAN_YIELD Buffer decompress(PoolTag tag = POOL_ASYNC_BUFFER, uint32_t alignment = 16) const;

    // decompress content of the buffer into provided memory
    CAN_YIELD bool decompressInto(uint64_t offset, void* targetMemory, uint64_t targetSize) const;

    //---

    // extract raw data
    void extract(Buffer& outCompressedData, CompressionType& outCompressiontType) const;

private:
    uint64_t m_size = 0;
    uint64_t m_crc = 0;
    CompressionType m_compression = CompressionType::Uncompressed;
    Buffer m_data;
};

//--

END_BOOMER_NAMESPACE()
