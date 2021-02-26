/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationBitReader.h"

//#define DEBUG_BIT_READER

BEGIN_BOOMER_NAMESPACE_EX(replication)

///---

BitReader::BitReader() = default;
BitReader::~BitReader() = default;

BitReader::BitReader(const void* data, uint32_t bitCount)
{
    reset(data, bitCount);
}

void BitReader::reset(const void* data, uint32_t bitCount)
{
    m_blockStart = (WORD*)data;
    m_blockPos = (WORD*)data;
    m_blockEnd = (WORD*)data + (bitCount + WORD_SIZE-1) / WORD_SIZE;

    m_bitPos = 0;
    m_bitIndex = 0;
    m_bitCapacity = bitCount;
}

void BitReader::align()
{
#ifdef DEBUG_BIT_READER
    TRACE_INFO("BitReader: Aligning {}->{}", m_bitPos, Align(m_bitPos, 8U));
#endif

    m_bitPos = Align(m_bitPos, 8U);
    m_bitIndex = Align(m_bitIndex, 8U);

    if (m_bitIndex == WORD_SIZE)
    {
        m_blockPos += 1;
        m_bitIndex = 0;
    }

    DEBUG_CHECK(m_bitPos % WORD_SIZE == m_bitIndex);
}

bool BitReader::readBit(bool& outValue)
{
    if (!checkSize(1))
        return false;

    auto mask  = 1U << m_bitIndex;
    auto val  = (*m_blockPos & mask) != 0;

    ++m_bitPos;
    if (WORD_SIZE == ++m_bitIndex)
    {
        m_blockPos += 1;
        m_bitIndex = 0;
    }

#ifdef DEBUG_BIT_READER
    TRACE_INFO("BitReader: Reading bit {} at {}", val, m_bitPos - 1);
#endif

    outValue = val;
    return true;
}

bool BitReader::readBits(uint32_t numBits, BitReader::WORD& outWord)
{
    if (!checkSize(numBits))
        return false;

    auto bitsLeft  = std::min<uint32_t>(WORD_SIZE - m_bitIndex, numBits);

    // write what fits
    auto value  = (*m_blockPos >> m_bitIndex);
    if (bitsLeft != WORD_SIZE)
            value &= ((1U << bitsLeft) - 1);

    m_bitIndex += bitsLeft;
    m_bitPos += bitsLeft;

    // advance
    if (WORD_SIZE == m_bitIndex)
    {
        m_blockPos += 1;
        m_bitIndex = 0;
    }

    // write rest
    if (auto bitsRight  = numBits - bitsLeft)
    {
        ASSERT(bitsRight < WORD_SIZE);

        auto rest  = *m_blockPos;

        if (bitsRight != WORD_SIZE)
            rest &= ((1U << bitsRight) - 1);

        value |= rest << bitsLeft;
        m_bitIndex += bitsRight;
        m_bitPos += bitsRight;
    }

#ifdef DEBUG_BIT_READER
    TRACE_INFO("BitReader: Reading {} bits {} at {}", numBits, Hex(value), m_bitPos - numBits);
#endif

    outWord = value;
    return true;
}

bool BitReader::readBlock(void* data, uint32_t dataSize)
{
    if (0 != (m_bitPos & 7))
        return false;

    if (!checkSize(dataSize*8))
        return false;

#ifdef DEBUG_BIT_READER
    TRACE_INFO("BitReader: Reading blob {} ({} bits) at {}", dataSize, dataSize*8, m_bitPos);
#endif

    auto readPos  = (const uint8_t*)m_blockStart + (m_bitPos / 8);
    memcpy(data, readPos, dataSize);
    m_bitPos += dataSize * 8;
    m_bitIndex = m_bitPos % WORD_SIZE;
    m_blockPos = m_blockStart + (m_bitPos / WORD_SIZE);

    return true;
}

bool BitReader::readAdaptiveNumber(BitReader::WORD& outWord)
{
    WORD value = 0;

    uint32_t numBits = 4;
    while (numBits < WORD_SIZE)
    {
        bool bit = false;
        if (!readBit(bit))
            return false;
        if (!bit)
            break;
        numBits += numBits < 16 ? 4 : 8;
    }

    return readBits(numBits, outWord);
}

///---

END_BOOMER_NAMESPACE_EX(replication)
