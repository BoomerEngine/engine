/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(replication)

/// reader of bits
class CORE_REPLICATION_API BitReader : public NoCopy
{
public:
    typedef uint32_t WORD;

    BitReader();
    BitReader(const void* data, uint32_t bitCount);
    ~BitReader();

    //--

    // get written data
    INLINE const WORD* data() const { return m_blockStart; }

    /// get current bit we are reading
    INLINE uint32_t bitPos() const { return m_bitPos; }

    /// get number of written bits
    INLINE uint32_t bitSize() const { return m_bitCapacity; }

    //--

    /// reset reader
    void reset(const void* data, uint32_t bitCount);

    /// align to byte boundary
    void align();

    /// write single bit, returns false if we can't read the data
    bool readBit(bool& outData);

    /// write multiple bits
    bool readBits(uint32_t numBits, WORD& outData);

    /// write string of data (requires being aligned to byte boundary)
    bool readBlock(void* data, uint32_t dataSize);

    /// write adaptive number, usually an array count/object ID
    bool readAdaptiveNumber(WORD& outData);

private:
    static const uint32_t MAX_INTERNAL_WORDS = 32;
    static const uint32_t WORD_SIZE = sizeof(WORD) * 8;

    WORD* m_blockStart = nullptr;
    WORD* m_blockEnd = nullptr;
    WORD* m_blockPos = nullptr;

    uint32_t m_bitPos = 0;
    uint32_t m_bitCapacity = 0;
    uint32_t m_bitIndex = 0;

    INLINE bool checkSize(uint32_t size) const
    {
        return m_bitPos + size <= m_bitCapacity;
    }
};

END_BOOMER_NAMESPACE_EX(replication)
