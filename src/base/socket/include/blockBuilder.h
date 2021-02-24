/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: raw #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base::socket)

//--

// a helper class to build a packet form few parts that can be written independently
class BASE_SOCKET_API BlockBuilder : public NoCopy
{
public:
    BlockBuilder();
    ~BlockBuilder();

    // reset state
    void clear();

    //--

    // get the block part (for gluing stuff)
    INLINE operator BlockPart() const { return BlockPart(data(), size()); }

    // get size of the data written so far
    INLINE uint32_t size() const { return (uint32_t)(m_curPos - m_startPos); }

    // get pointer to current data
    INLINE uint8_t* data() { return m_startPos; }

    // get pointer to current data
    INLINE const uint8_t* data() const { return m_startPos; }

    // reserve memory for data
    INLINE void reserve(uint32_t size)
    {
        if (m_curPos + size > m_endPos)
            grow(size);
    }

    // write data
    INLINE void write(const void* data, uint32_t size)
    {
        if (m_curPos + size > m_endPos)
            grow(size);

        memcpy(m_curPos, data, size);
        m_curPos += size;
    }

    // write data without checks - can stop memory
    INLINE void writeUnchecked(const void* data, uint32_t size)
    {
        ASSERT(m_curPos + size <= m_endPos);
        memcpy(m_curPos, data, size);
        m_curPos += size;
    }

private:
    static const uint32_t MAX_INTERNAL_SIZE = 2048;

    uint8_t* m_startPos;
    uint8_t* m_curPos;
    uint8_t* m_endPos;

    uint8_t m_internalBuffer[MAX_INTERNAL_SIZE];

    void grow(uint32_t additionalSizeNeeded);
};

//--

END_BOOMER_NAMESPACE(base::socket)