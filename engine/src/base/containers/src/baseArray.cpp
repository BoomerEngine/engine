/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers\dynamic #]
***/

#include "build.h"
#include "baseArray.h"

namespace base
{
    //--

    void BaseArray::suckFrom(BaseArray& other)
    {
        ASSERT_EX(m_data == nullptr, "Array is not empty");
        ASSERT_EX(other.empty() || other.isLocal(), "Source array is not local");

        m_data = other.m_data;
        m_size = other.m_size;
        m_capacityAndFlags = other.m_capacityAndFlags;

        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacityAndFlags = 0;
    }

    void BaseArray::checkIndex(uint32_t index) const
    {
        ASSERT_EX(index < size(), "Array index out of range");
    }

    void BaseArray::checkIndexRange(uint32_t index, uint32_t count) const
    {
        ASSERT_EX((index + count) <= size(), "Array range out of range");
    }

    uint32_t BaseArray::changeSize(uint32_t newSize)
    {
        ASSERT_EX(newSize <= capacity(), "Array does not have capacity for given size")
        auto oldSize = m_size;
        m_size = newSize;
        return oldSize;
    }

	void BaseArray::makeLocal(mem::PoolID poolId, uint32_t minimumCapacity, uint32_t elementSize, uint32_t elementAlignment, const char* debugTypeName)
	{
		if (m_capacityAndFlags & FLAG_EXTERNAL)
		{
			// we are making an empty array local, do not allocate memory
			if (minimumCapacity == 0)
			{
				m_data = nullptr;
				m_capacityAndFlags = 0;
			}
			else
			{
				// calculate best capacity based on the number of elements
				uint32_t newCapacity = 0;
				while (minimumCapacity > newCapacity)
					newCapacity = CalcNextCapacity(newCapacity, elementSize);

				// allocate a local data buffer
				auto newData  = mem::AllocateBlock(poolId, elementSize * (size_t)newCapacity, elementAlignment, __FILE__, __LINE__, debugTypeName);

				// copy data from old buffer to new one
				uint32_t copyDataSize = elementSize * std::min<uint32_t>(m_size, newCapacity);
				memcpy(newData, m_data, copyDataSize);

				// fill previous buffer with pattern to indicate it was cleared
#ifndef BUILD_RELEASE
				memset(m_data, 0xCD, elementSize * capacity());
#endif

				// update date
				m_data = newData;
				m_capacityAndFlags = newCapacity; // no longer external buffer
			}
		}		
	}

    uint32_t BaseArray::changeCapacity(mem::PoolID poolId, uint32_t newCapacity, uint32_t elementSize, uint32_t elementAlignment, const char* debugTypeName)
    {
        ASSERT_EX(0 == (m_capacityAndFlags & FLAG_FIXED_CAPACITY), "Array buffer cannot be resized");
		ASSERT_EX(0 == (m_capacityAndFlags & FLAG_EXTERNAL), "Cannot change capacity of an array with external buffer");

        auto oldCapacity = capacity();

        // free memory
        if (newCapacity == 0)
        {
            mem::FreeBlock(m_data);
            m_data = nullptr;
            m_capacityAndFlags = 0;
            return oldCapacity;
        }

		if (oldCapacity != newCapacity)
        {
            // just resize buffer
            m_data = mem::ResizeBlock(poolId, m_data, (size_t)newCapacity * elementSize, elementAlignment, __FILE__, __LINE__, debugTypeName);
            m_capacityAndFlags = newCapacity; // no longer external buffer
        }

        return oldCapacity;
    }

    uint32_t BaseArray::CalcNextCapacity(uint32_t currentCapacity, uint32_t elementSize)
    {
        // small allocations are shit, prevent them
        if (currentCapacity == 0)
            return std::max<uint32_t>(1, 64 / elementSize);

        // grow with x1.5 scale
        return std::max<uint32_t>(currentCapacity+1, (currentCapacity * 3) / 2);
    }

    //--

} // base