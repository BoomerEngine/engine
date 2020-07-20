/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#include "build.h"
#include "baseArray.h"

namespace base
{
    //--

    BaseArrayBuffer::~BaseArrayBuffer()
    {
        release();
    }

    void BaseArrayBuffer::resize(Count newCapcity, uint64_t currentMemorySize, uint64_t newMemorySize, uint64_t memoryAlignment)
    {
        if (newCapcity == 0)
        {
            release();
        }
        else if (m_flagOwned)
        {
            m_ptr = mem::ResizeBlock(POOL_CONTAINERS, m_ptr, newMemorySize, memoryAlignment);
        }
        else
        {
            auto* newPtr = mem::ResizeBlock(POOL_CONTAINERS, nullptr, newMemorySize, memoryAlignment);
            memcpy(newPtr, m_ptr, std::min<uint64_t>(currentMemorySize, newMemorySize));
            m_ptr = newPtr;
        }

        m_capacity = newCapcity;
        m_flagOwned = true;
    }

    void BaseArrayBuffer::release()
    {
        if (m_flagOwned)
            MemFree(m_ptr);

        m_flagOwned = true;
        m_capacity = 0;
        m_ptr = nullptr;
    }

    //--

#ifndef BUILD_RELEASE
    void BaseArray::checkIndex(Index index) const
    {
        ASSERT_EX(index >= 0 && index <= lastValidIndex(), "Array index out of range");
    }

    void BaseArray::checkIndexRange(Index index, Count count) const
    {
        ASSERT_EX((index >= 0) && (index + count) <= size(), "Array range out of range");
    }
#endif

    Count BaseArray::changeSize(Count newSize)
    {
        ASSERT_EX(newSize <= capacity(), "Array does not have capacity for given size")
        auto oldSize = m_size;
        m_size = newSize;
        return oldSize;
    }

    Count BaseArray::changeCapacity(Count newCapacity, uint64_t currentMemorySize, uint64_t newMemorySize, uint64_t memoryAlignment, const char* typeNameInfo)
    {
        auto oldCapacity = capacity();
        m_buffer.resize(newCapacity, currentMemorySize, newMemorySize, memoryAlignment);
        return oldCapacity;
    }

    // NOTE: this is in CPP ONLY so we can tinker with it and NOT recompile everything
    
    uint64_t BaseArray::CalcNextBufferSize(uint64_t currentSize)
    {
        if (currentSize < 64)
            return 64; // smallest sensible allocation for array

        return (currentSize * 3) / 2;
    }

} // base