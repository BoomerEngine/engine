/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

namespace base
{
    //--

    ALWAYS_INLINE BaseArrayBuffer::BaseArrayBuffer()
    {
        m_capacity = 0;
        m_flagOwned = true;
    }

    ALWAYS_INLINE BaseArrayBuffer::BaseArrayBuffer(void* externalPointer, Count capacity, bool canResize)
        : m_ptr(externalPointer)
    {
        m_capacity = capacity;
        m_flagOwned = canResize;
    }

    ALWAYS_INLINE BaseArrayBuffer::BaseArrayBuffer(BaseArrayBuffer&& other)
    {
        m_ptr = other.m_ptr;
        m_capacity = other.m_capacity;
        m_flagOwned = other.m_flagOwned;

        other.m_ptr = nullptr;
        other.m_capacity = 0;
        other.m_flagOwned = true;
    }

    ALWAYS_INLINE BaseArrayBuffer& BaseArrayBuffer::operator=(BaseArrayBuffer&& other)
    {
        if (this != &other)
        {
            m_ptr = other.m_ptr;
            m_capacity = other.m_capacity;
            m_flagOwned = other.m_flagOwned;

            other.m_ptr = nullptr;
            other.m_capacity = 0;
            other.m_flagOwned = true;
        }

        return *this;
    }

    ALWAYS_INLINE void* BaseArrayBuffer::data()
    {
        return m_ptr;
    }

    ALWAYS_INLINE const void* BaseArrayBuffer::data() const
    {
        return m_ptr;
    }

    ALWAYS_INLINE Count BaseArrayBuffer::capacity() const
    {
        return m_capacity;
    }

    ALWAYS_INLINE bool BaseArrayBuffer::owned() const
    {
        return m_flagOwned;
    }

    ALWAYS_INLINE bool BaseArrayBuffer::empty() const
    {
        return m_capacity == 0;
    }

    //--

    ALWAYS_INLINE BaseArray::BaseArray(BaseArrayBuffer&& buffer)
        : m_size(0)
        , m_buffer(std::move(buffer))
    {}

    ALWAYS_INLINE BaseArray::~BaseArray()
    {
		DEBUG_CHECK_EX(0 == m_size, "Array still has elements that will now leak");
    }

    ALWAYS_INLINE void* BaseArray::data()
    {
        return m_buffer.data();
    }

    ALWAYS_INLINE const void* BaseArray::data() const
    {
        return m_buffer.data();
    }

    ALWAYS_INLINE bool BaseArray::empty() const
    {
        return m_size == 0;
    }

    ALWAYS_INLINE bool BaseArray::full() const
    {
        return m_size == capacity();
    }

    ALWAYS_INLINE bool BaseArray::owned() const
    {
        return m_buffer.owned();
    }

    ALWAYS_INLINE uint32_t BaseArray::size() const
    {
        return m_size;
    }

    ALWAYS_INLINE Index BaseArray::lastValidIndex() const
    {
        return (Index)m_size - 1;
    }

    ALWAYS_INLINE uint32_t BaseArray::capacity() const
    {
        return m_buffer.capacity();
    }

    ALWAYS_INLINE BaseArrayBuffer& BaseArray::buffer()
    {
        return m_buffer;
    }

    ALWAYS_INLINE const BaseArrayBuffer& BaseArray::buffer() const
    {
        return m_buffer;
    }

    //--

} // base