/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationBitWriter.h"

//#define DEBUG_BIT_WRITER

BEGIN_BOOMER_NAMESPACE(base::replication)

///---

BitWriter::BitWriter()
{
    m_blockStart = m_internalBuffer;
    m_blockEnd = m_blockStart + MAX_INTERNAL_WORDS;
    m_blockPos = m_internalBuffer;
    memzero(m_internalBuffer, sizeof(m_internalBuffer));

    m_bitCapacity = MAX_INTERNAL_WORDS * WORD_SIZE;
}

BitWriter::BitWriter(void* externalMemory, uint32_t bitCapacity, bool releaseMemory)
{
    ASSERT((bitCapacity % WORD_SIZE) == 0);

    m_blockStart = (WORD*)externalMemory;
    m_blockPos = m_blockStart;
    m_blockEnd = m_blockStart + (bitCapacity / WORD_SIZE);
    ASSERT(*m_blockPos == 0);

    m_bitCapacity = bitCapacity;
}

BitWriter::~BitWriter()
{
    if (m_releaseMemory)
        mem::GlobalPool<POOL_NET_REPLICATION, WORD>::Free(m_blockStart);
}

void BitWriter::clear()
{
    m_bitIndex = 0;
    m_bitPos = 0;
    m_blockPos = m_blockStart;
}

void BitWriter::reserve(uint32_t numBits)
{
    if (m_bitPos + numBits > m_bitCapacity)
    {
        auto newCap = m_bitCapacity * 2;
        while (numBits > newCap)
            newCap *= 2;

        grow(newCap / WORD_SIZE);
        ASSERT(m_bitPos + numBits <= m_bitCapacity);
    }
}

void BitWriter::advance()
{
	ASSERT(m_blockPos != m_blockEnd);

    if (m_bitIndex == WORD_SIZE)
	{
		m_blockPos += 1;

		if (m_blockPos == m_blockEnd)
			reserve(1024);

        ASSERT(*m_blockPos == 0);
        m_bitIndex = 0;
    }

    DEBUG_CHECK(m_bitPos % WORD_SIZE == m_bitIndex);
}

void BitWriter::writeBit(bool val)
{
    ASSERT_EX(m_bitPos < m_bitCapacity, "Run out of space, forget to reserve() ?");

#ifdef DEBUG_BIT_WRITER
    TRACE_INFO("BitWriter: Writing bit {} at {}", val, m_bitPos);
#endif

    advance();

    auto mask = 1U << m_bitIndex;
    ASSERT(0 == (mask & *m_blockPos))

    if (val)
        *m_blockPos |= mask;

    ++m_bitPos;
    ++m_bitIndex;
}

void BitWriter::writeBits(WORD unsafeValue, uint32_t numBits)
{
    ASSERT_EX(m_bitPos + numBits <= m_bitCapacity, "Run out of space, forget to reserve() ?");

    // advance to next word
    advance();

    // how many bits can we write in current word ?
    auto bitsLeft = std::min<uint32_t>(WORD_SIZE - m_bitIndex, numBits);

    // make the value safe
    auto value = unsafeValue;
    if (numBits != WORD_SIZE)
        value &= (((WORD)1) << numBits) - 1;

#ifdef DEBUG_BIT_WRITER
    TRACE_INFO("BitWriter: Writing {} bits {} at {}", numBits, Hex(value), m_bitPos);
#endif

    // check that we have zeros where we write
    auto maskLeft = ((1ULL << bitsLeft) - 1) << m_bitIndex;
    ASSERT((*m_blockPos & maskLeft) == 0);

    // write what fits
    *m_blockPos |= (value << m_bitIndex);
    m_bitIndex += bitsLeft;
    m_bitPos += bitsLeft;

    // write rest
    if (auto bitsRight = numBits - bitsLeft)
    {
        ASSERT(WORD_SIZE == m_bitIndex);
        advance();

        auto maskRight = ((1ULL << bitsRight) - 1) << m_bitIndex;
        ASSERT((*m_blockPos & maskRight) == 0);

        *m_blockPos |= (value >> bitsLeft);
        m_bitIndex += bitsRight;
        m_bitPos += bitsRight;
    }
}

void BitWriter::align()
{
#ifdef DEBUG_BIT_WRITER
    TRACE_INFO("BitWriter: Aligning {}->{}", m_bitPos, Align(m_bitPos, 8U));
#endif

    m_bitIndex = Align(m_bitIndex, 8U);
    m_bitPos = Align(m_bitPos, 8U);
}

void BitWriter::writeBlock(const void* data, uint32_t dataSize)
{
#ifdef DEBUG_BIT_WRITER
    TRACE_INFO("BitWriter: Writing blob {} ({} bits) at {}", dataSize, dataSize*8, m_bitPos);
#endif
    advance();

    ASSERT_EX((m_bitPos & 7) == 0, "Bit stream must be aligned to byte boundary to write a data block");
    reserve(dataSize * 8);

    auto writePos  = ((uint8_t*)m_blockStart) + (m_bitPos / 8);
    memcpy(writePos, data, dataSize);
    m_bitPos += dataSize * 8;
    m_bitIndex = m_bitPos % WORD_SIZE;
    m_blockPos = m_blockStart + (m_bitPos / WORD_SIZE);
}

void BitWriter::writeAdaptiveNumber(WORD value)
{
#ifdef DEBUG_BIT_WRITER
    TRACE_INFO("BitWriter: Writing adaptive number {} at {}", value, m_bitPos);
#endif

    uint32_t numBits = 4;
    while (numBits < WORD_SIZE)
    {
        auto maxValue = ((WORD)1) << numBits;
        if (value < maxValue)
        {
            writeBit(0);
            break;
        }

        writeBit(1);
        numBits += numBits < 16 ? 4 : 8;
    }

    writeBits(value, numBits);
}

void BitWriter::grow(uint32_t requiredWords)
{
    if (m_blockPos + requiredWords > m_blockEnd)
    {
#ifdef DEBUG_BIT_WRITER
        TRACE_INFO("BitWriter: Growing storage {}->{}", m_bitCapacity, requiredWords * WORD_SIZE);
#endif

        auto oldData  = m_blockStart;

        auto newData  = mem::GlobalPool<POOL_NET_REPLICATION, WORD>::AllocN(requiredWords);
        memzero(newData, sizeof(WORD) * requiredWords);
        memcpy(newData, m_blockStart, (m_blockEnd - m_blockStart) * sizeof(WORD));

        m_blockPos = newData + (m_blockPos - m_blockStart);
        m_blockEnd = newData + requiredWords;
        m_blockStart = newData;

        if (m_releaseMemory)
            mem::GlobalPool<POOL_NET_REPLICATION, WORD>::Free(oldData);

        m_bitCapacity = requiredWords * WORD_SIZE;
        m_releaseMemory = true;
    }
}

///---

END_BOOMER_NAMESPACE(base::replication)
