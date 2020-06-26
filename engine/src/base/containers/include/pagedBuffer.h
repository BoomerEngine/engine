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

    /// Large buffer that can support effective allocation of elements up to the memory exhaustion
    /// The elements are placed on the pages allocated from the supplied page allocator
    /// NOTE: page size must be at least of the sizeof(T)
    class BASE_CONTAINERS_API PagedBufferBase : public base::NoCopy
    {
    public:
        PagedBufferBase(uint32_t size, uint32_t alignment, mem::PoolID poolID = POOL_TEMP); // use shared page allocator
        PagedBufferBase(uint32_t size, uint32_t alignment, mem::PageAllocator& pageAllocator); // use given page allocator
        PagedBufferBase(uint32_t size, uint32_t alignment, mem::PoolID poolID, uint32_t pageSize); // use dedicated (owned) page allocator with given size of page
        ~PagedBufferBase();

        //--

        INLINE bool empty() const { return m_numElements == 0; }

        INLINE uint64_t size() const { return m_numElements; }
        INLINE uint32_t elementSize() const { return m_elementSize; }
        INLINE uint64_t dataSize() const { return m_elementSize * m_numElements; }

        INLINE uint32_t pageSize() const { return m_pageSize; }
        INLINE uint32_t pageCount() const { return m_pageCount; }

        //--

        // allocate new element
        void* allocSingle();

        //! allocate batch of N elements, returns pointer to the first one and number of elements allocated
        //! this function should be used in a loop, similar to IO read/write
        //! NOTE: although the paged buffer can hold more than 4GB of memory the allocations must be done in batches smaller than 4GB
        void* allocateBatch(uint64_t memorysize, uint32_t elementCount, uint32_t& outNumAllocated);

        //---

        // free all pages
        void clear();

        //! write data to buffer
        void write(const void* data, uint64_t elementCountToWrite);

        //! fill buffer with copies of the same element
        void fill(const void* templateData, uint64_t elementCountToWrite);

        //! copy all data to outside buffer
        void copy(void* ptr, uint64_t outputBufferSize) const;

        //! append other paged buffer
        void append(const PagedBufferBase& buffer);

        //--

        // export data to buffer
        Buffer toBuffer(mem::PoolID pool = POOL_DEFAULT) const;

    protected:
        uint8_t* m_writePtr = nullptr; // current write pointer
        uint8_t* m_writeEndPtr = nullptr; // end of page

        uint32_t m_elementSize = 0;
        uint32_t m_elementAlignment = 0;
        uint64_t m_numElements = 0; // total element count so far

        int m_elementsLeftOnPage = 0; // elements left to write on current page, all other pages are full

        uint8_t* m_writeStartPtr = nullptr; // start of page

        uint32_t m_pageSize = 0;
        uint32_t m_pageCount = 0;

        uint64_t m_writeOffsetSoFar = 0;

        //--

        struct Page
        {
            Page* next = nullptr;
            Page* prev = nullptr;
            uint32_t dataSize = 0;
            uint64_t offsetSoFar = 0;
        };

        Page* m_pageList = nullptr;
        Page* m_pageHead = nullptr;

        //--

        mem::PageAllocator* m_allocator = nullptr;
        bool m_ownsAllocator = false;

        //--

        void allocPage();
    };

    //---

    /// typed paged buffer
    template< typename T >
    class PagedBuffer : public PagedBufferBase
    {
    public:
        INLINE PagedBuffer(mem::PoolID poolID = POOL_TEMP); // use shared page allocator
        INLINE PagedBuffer(mem::PageAllocator& pageAllocator); // use given page allocator
        INLINE PagedBuffer(mem::PoolID poolID, uint32_t pageSize); // use dedicated (owned) page allocator with given size of page
        INLINE ~PagedBuffer();

        // allocate new element
        INLINE T* allocSingle();

        //! allocate batch of N elements, returns pointer to the first one and number of elements allocated
        //! this function should be used in a loop, similar to IO read/write
        //! NOTE: although the paged buffer can hold more than 4GB of memory the allocations must be done in batches smaller than 4GB
        INLINE T* allocateBatch(uint32_t elementCount, uint32_t& outNumAllocated);

        //! visit all elements in order
        template< typename F >
        INLINE void forEach(const F& func) const;
    };

    //---

} // base

#include "pagedBuffer.inl"