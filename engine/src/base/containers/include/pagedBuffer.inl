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

    //---

    template< typename T >
    INLINE PagedBuffer<T>::PagedBuffer(mem::PoolID poolID /*= POOL_TEMP*/)
        : PagedBufferBase(sizeof(T), alignof(T), poolID)
    {}

    template< typename T >
    INLINE PagedBuffer<T>::PagedBuffer(mem::PageAllocator& pageAllocator)
    : PagedBufferBase(sizeof(T), alignof(T), pageAllocator)
    {}

    template< typename T >
    INLINE PagedBuffer<T>::PagedBuffer(mem::PoolID poolID, uint32_t pageSize)
        : PagedBufferBase(sizeof(T), alignof(T), poolID, pageSize)
    {}

    template< typename T >
    INLINE PagedBuffer<T>::~PagedBuffer()
    {}

    template< typename T >
    INLINE T* PagedBuffer<T>::allocSingle()
    {
        return (T*)PagedBufferBase::allocSingle();
    }

    template< typename T >
    INLINE T* PagedBuffer<T>::allocateBatch(uint32_t elementCount, uint32_t& outNumAllocated)
    {
        return (T*)PagedBufferBase::allocateBatch(sizeof(T) * elementCount, elementCount, outNumAllocated);
    }

    template< typename T >
    template< typename F >
    INLINE void PagedBuffer<T>::forEach(const F& func) const
    {
        if (auto* page = m_pageHead)
        {
            while (page)
            {
                auto* pagePayload = AlignPtr((char*)page + sizeof(Page), m_elementAlignment);
                std::for_each((const T*)pagePayload, (const T*)(pagePayload + page->dataSize), func);
                page = page->next;
            }
        }

        if (m_writePtr > m_writeStartPtr)
            std::for_each((const T*)m_writeStartPtr, (const T*)m_writePtr, func);
    }

    //---

} // base