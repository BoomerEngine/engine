/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers\dynamic #]
***/

#pragma once

namespace base
{

    //--

    INLINE BaseArray::BaseArray(void* externalBuffer, uint32_t externalCapacity)
        : m_size(0)
        , m_capacityAndFlags(externalCapacity | FLAG_EXTERNAL)
        , m_data(externalBuffer)
    {}

    INLINE BaseArray::~BaseArray()
    {
		DEBUG_CHECK_EX(0 == m_size, "Array still has elements that will now leak");
        DEBUG_CHECK_EX(!m_data || !isLocal(), "Destructor called on non-empty array");
    }

    INLINE void* BaseArray::data()
    {
        return m_data;
    }

    INLINE void BaseArray::adjustSize(uint32_t newSize)
    {
        m_size = newSize;
    }

    INLINE const void* BaseArray::data() const
    {
        return m_data;
    }

    INLINE bool BaseArray::empty() const
    {
        return m_size == 0;
    }

    INLINE bool BaseArray::full() const
    {
        return m_size == capacity();
    }

    INLINE uint32_t BaseArray::size() const
    {
        return m_size;
    }

    INLINE uint32_t BaseArray::capacity() const
    {
        return m_capacityAndFlags & FLAG_MASK;
    }

    INLINE const void* BaseArray::elementPtr(uint32_t index, uint32_t elementSize) const
    {
        checkIndex(index);
        return base::OffsetPtr(m_data, index * elementSize);
    }

    INLINE void* BaseArray::elementPtr(uint32_t index, uint32_t elementSize)
    {
        checkIndex(index);
        return base::OffsetPtr(m_data, index * elementSize);
    }

	INLINE bool BaseArray::isLocal() const
	{
		return 0 == (m_capacityAndFlags & FLAG_EXTERNAL);
	}

	INLINE bool BaseArray::canResize() const
	{
		return 0 == (m_capacityAndFlags & FLAG_FIXED_CAPACITY);
	}

} // base