/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: raw #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(socket)

//--

// a block of memory for network use
class CORE_SOCKET_API Block : public NoCopy
{
public:
    // get size of the data in the block
    INLINE uint32_t dataSize() const { return m_currentSize; }

    // get data
    INLINE const uint8_t* data() const { return m_ptr; }

    // get data
    INLINE uint8_t* data() { return m_ptr; }

    //--

    // we are done with the block, release it back to where it came from
    void release();

    // eat header/tail data, effectively creates a block for the sub-part of the messages
    void shrink(uint32_t skipFront, uint32_t newSize = INDEX_MAX);

private:
    uint8_t* m_ptr = nullptr;
    uint32_t m_currentSize = 0;
    uint32_t m_totalSize = 0;
    BlockAllocator* m_allocator = nullptr;

    friend class BlockAllocator;
};

//--

END_BOOMER_NAMESPACE_EX(socket)
