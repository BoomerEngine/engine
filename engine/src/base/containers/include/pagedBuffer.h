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

    //---

    /// Large buffer that can support effective allocation of elements up to the memory exhaustion
    /// The elements are placed on the pages allocated from the supplied page allocator thus there's nothing there to constantly resize as the buffer grows
    /// NOTE: page size must be at least of the sizeof(T)
	/// NOTE: this container allocates from page allocator
    class BASE_CONTAINERS_API PagedBuffer : public base::NoCopy
    {
    public:
        PagedBuffer(uint32_t alignment, PoolTag poolID = POOL_TEMP); // use shared page allocator
        PagedBuffer(uint32_t alignment, mem::PageAllocator& pageAllocator); // use given page allocator
        PagedBuffer(uint32_t alignment, PoolTag poolID, uint32_t pageSize); // use dedicated (owned) page allocator with given size of page
        ~PagedBuffer();

        //--

        INLINE bool empty() const { return m_numBytes == 0; }

        INLINE uint64_t dataSize() const { return m_numBytes; }

        INLINE uint32_t pageSize() const { return m_pageSize; }
        INLINE uint32_t pageCount() const { return m_pageCount; }

        //--

        // allocate storage for N bytes where N is small compared to page size (ie. simple value like "float", "double" etc)
		// NOTE: this does not respect the alignment
		// NOTE: this is the fastest way to go around
        uint8_t* allocSmall(uint32_t memorySize);

		// allocate small object
		template< typename T >
		INLINE T* allocSmall()
		{
			return (T*)allocSmall(sizeof(T));
		}


        //! allocate storage for N bytes where N is potentially larger than capacity of a single page (thus it has to be split over multiple pages)
		//! returns pointer to the first one and number of elements allocated
        //! this function should be used in a loop, similar to IO read/write
        //! NOTE: although the paged buffer can hold more than 4GB of memory the allocations must be done in batches smaller than 4GB
		//! NOTE: only multiple of "elementSize" are allocated thus making sure that a single allocation element does not span multiple pages
		uint8_t* allocateBatch(uint64_t memorySize, uint32_t elementSize, uint32_t& outNumAllocated);

        //---

        // free all pages
        void clear();

		//! write small (<1/16 of page) data to buffer
		INLINE void writeSmall(const void* data, uint32_t size);

        //! write large data to buffer
        void writeLarge(const void* data, uint64_t size);

		//--

        //! copy all data to outside buffer
        void copy(void* ptr, uint64_t outputBufferSize) const;

        //! append other paged buffer
        void append(const PagedBuffer& buffer);

        //--

        // export data to buffer
		// TODO: this should be removed, otherwise it means there's potentially memory space for the whole copy of the buffer - this is pointless
        Buffer toBuffer(PoolTag pool = POOL_DEFAULT) const;

		//--

		// iterate pages (read only version), visits each allocated memory page, can be used to write/stream compress the buffer
		void iteratePages(const std::function<void(const void* pageData, uint32_t pageDataSize, uint64_t totalOffset)>& func) const;

		// iterate pages (writable version), visits each allocated memory page, can be used to write/stream compress the buffer (read only version)
		void iteratePages(const std::function<void(void* pageData, uint32_t pageDataSize, uint64_t totalOffset)>& func);

		//--

		// TODO: IO support for directly writing paged buffer to disk

    protected:
        uint8_t* m_writePtr = nullptr; // current write pointer
        uint8_t* m_writeEndPtr = nullptr; // end of page
        uint32_t m_alignment = 0;

        uint64_t m_numBytes = 0; // total element count so far
        int m_bytesLeftOnPage = 0; // elements left to write on current page, all other pages are full

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
    class PagedBufferTyped : public PagedBuffer
    {
    public:
        INLINE PagedBufferTyped(PoolTag poolID = POOL_TEMP); // use shared page allocator
        INLINE PagedBufferTyped(mem::PageAllocator& pageAllocator); // use given page allocator
        INLINE PagedBufferTyped(PoolTag poolID, uint32_t pageSize); // use dedicated (owned) page allocator with given size of page
        INLINE ~PagedBufferTyped();

		// return size (in element) of the buffer
		INLINE uint32_t size() const;

        // allocate one new element
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