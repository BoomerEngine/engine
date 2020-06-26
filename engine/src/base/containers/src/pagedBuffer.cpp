/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers\dynamic #]
***/

#include "build.h"
#include "pagedBuffer.h"

#include "base/memory/include/pageAllocator.h"

namespace base
{
    ///--

    PagedBufferBase::PagedBufferBase(uint32_t size, uint32_t alignment, mem::PoolID poolID /*= POOL_TEMP*/)
        : m_allocator(&mem::PageAllocator::GetDefaultAllocator(poolID))
        , m_elementAlignment(alignment)
        , m_elementSize(size)
    {
        m_pageSize = m_allocator->pageSize();
    }

    PagedBufferBase::PagedBufferBase(uint32_t size, uint32_t alignment, mem::PageAllocator& pageAllocator)
        : m_allocator(&pageAllocator)
        , m_elementAlignment(alignment)
        , m_elementSize(size)
    {
        m_pageSize = m_allocator->pageSize();
    }

    PagedBufferBase::PagedBufferBase(uint32_t size, uint32_t alignment, mem::PoolID poolID, uint32_t pageSize)
        : m_elementAlignment(alignment)
        , m_elementSize(size)
    {
        m_allocator = MemNew(mem::PageAllocator);
        m_allocator->initialize(poolID, pageSize);
        m_pageSize = m_allocator->pageSize();
        m_ownsAllocator = true;
    }

    PagedBufferBase::~PagedBufferBase()
    {
        clear();

        if (m_ownsAllocator)
            MemDelete(m_allocator);
    }

    void PagedBufferBase::clear()
    {
        auto* page = m_pageList;

        // reset state
        m_writePtr = nullptr;
        m_writeStartPtr = nullptr;
        m_writeEndPtr = nullptr;
        m_elementsLeftOnPage = 0;
        m_numElements = 0;
        m_writeStartPtr = nullptr;
        m_writeOffsetSoFar = 0;
        m_pageCount = 0;
        m_pageHead = nullptr;
        m_pageList = nullptr;

        // free full pages
        while (page)
        {
            auto* next = page->prev;
            m_allocator->freePage(page);
            page = next;
        }

        // do not put anything here !
    }

    void* PagedBufferBase::allocSingle()
    {
        auto* ret = m_writePtr;
        auto* retEnd = ret + m_elementSize;
        if (retEnd <= m_writeEndPtr)
        {
            m_writePtr = retEnd;
            DEBUG_CHECK_EX(m_elementsLeftOnPage >= 1, "Invalid count");
            m_elementsLeftOnPage -= 1;
            m_numElements += 1;
        }
        else
        {
            allocPage();

            ret = m_writePtr;
            m_writePtr += m_elementSize;
            DEBUG_CHECK_EX(m_elementsLeftOnPage >= 1, "Invalid count");
            m_elementsLeftOnPage -= 1;
            m_numElements += 1;
        }

        return ret;
    }

    void* PagedBufferBase::allocateBatch(uint64_t memorysize, uint32_t elementCount, uint32_t& outNumAllocated)
    {
        auto* ret = m_writePtr;
        auto* retEnd = ret + memorysize;
        if (retEnd <= m_writeEndPtr)
        {
            m_writePtr = retEnd;
            m_elementsLeftOnPage -= elementCount;
            DEBUG_CHECK_EX(m_elementsLeftOnPage >= 0, "Invalid count");
            outNumAllocated = elementCount; // all elements allocated
            m_numElements += outNumAllocated;
        }
        else if (m_elementsLeftOnPage > 0)
        {
            outNumAllocated = std::min<uint32_t>(elementCount, m_elementsLeftOnPage);
            m_elementsLeftOnPage -= outNumAllocated;
            DEBUG_CHECK_EX(m_elementsLeftOnPage == 0, "All memory from page used, we should have zero elements");
            m_writePtr = m_writeEndPtr;
            m_numElements += outNumAllocated;
        }
        else
        {
            allocPage();
            ret = (uint8_t*)allocateBatch(memorysize, elementCount, outNumAllocated);
        }

        return ret;
    }

    void PagedBufferBase::write(const void* data, uint64_t elementCountToWrite)
    {
        const auto* readPtr = (uint8_t*)data;
        const auto* readEndPtr = readPtr + (elementCountToWrite * m_elementSize);
        
        uint64_t left = elementCountToWrite;
        while (readPtr < readEndPtr)
        {
            uint32_t toCopy = std::min<uint64_t>(INDEX_MAX, left);
            uint64_t memorySize = toCopy * m_elementSize;
            void* writePtr = allocateBatch(memorySize, toCopy, toCopy);
            if (toCopy)
            {
                memorySize = toCopy * m_elementSize;
                memcpy(writePtr, readPtr, memorySize);
                readPtr += memorySize;
                left -= toCopy;
            }
        }
    }

    void PagedBufferBase::fill(const void* templateData, uint64_t elementCountToWrite)
    {
        uint64_t left = elementCountToWrite;
        while (left > 0)
        {
            uint32_t toCopy = std::min<uint64_t>(INDEX_MAX, left);
            uint64_t memorySize = toCopy * m_elementSize;
            uint8_t* writePtr = (uint8_t* )allocateBatch(memorySize, toCopy, toCopy);
            if (toCopy)
            {
                uint8_t* writePtrEnd = writePtr + (toCopy * m_elementSize);

                while (writePtr < writePtrEnd)
                {
                    memcpy(writePtr, templateData, m_elementSize);
                    writePtr += m_elementSize;
                }
                
                left -= toCopy;
            }
        }
    }

    void PagedBufferBase::append(const PagedBufferBase& buffer)
    {
        DEBUG_CHECK_EX(buffer.m_elementSize == m_elementSize, "Can only append data of the same size");
        if (auto* page = buffer.m_pageHead)
        {
            while (page)
            {
                auto* pagePayload = AlignPtr((char*)page + sizeof(Page), buffer.m_elementAlignment);
                auto pageElements = page->dataSize / buffer.m_elementSize;
                write(pagePayload, pageElements);

                page = page->next;
            }
        }

        if (buffer.m_writePtr > buffer.m_writeStartPtr)
        {
            auto numElems = (buffer.m_writePtr - buffer.m_writeStartPtr) / buffer.m_elementSize;
            write(buffer.m_writeStartPtr, numElems);
        }
    }

    void PagedBufferBase::copy(void* ptr, uint64_t size) const
    {
        auto* writePtr = (uint8_t*)ptr;
        auto* writeEndPtr = writePtr + size;

        // write full pages
        if (auto* page = m_pageList)
        {
            // write head page
            const auto dataSize = m_writePtr - m_writeStartPtr;
            DEBUG_CHECK_EX(writePtr + m_writeOffsetSoFar + dataSize <= writeEndPtr, "Write would write outside the memory bounds");
            memcpy(writePtr + m_writeOffsetSoFar, m_writeStartPtr, dataSize);

            // write rest of the pages
            page = page->prev;
            while (page)
            {
                DEBUG_CHECK_EX(writePtr + page->offsetSoFar + page->dataSize <= writeEndPtr, "Write would write outside the memory bounds");
                auto* pagePayload = AlignPtr((char*)page + sizeof(Page), m_elementAlignment);
                memcpy(writePtr + page->offsetSoFar, pagePayload, page->dataSize);
                page = page->prev;
            }
        }
    }

/*    bool PagedBufferBase::read(void*& outReadPtr, uint32_t& outCount, int& token) const
    {

        if ((token && token != END_OF_LIST_TOKEN) || (!token && m_pageListHead))
        {
            auto page = token ? (Page*)token : m_pageListHead;

            outReadPtr = page->elements;
            outCount = NUM_ELEMS_PER_PAGE - (page == m_pageListTail ? m_elementsLeftOnPage : page->elementsLeftOnPage);

            token = page->nextPage ? page->nextPage : END_OF_LIST_TOKEN;
            return true;
        }
        else
        {
            return false;
        }
    }*/

    void PagedBufferBase::allocPage()
    {
        // update current page
        if (m_writePtr > m_writeStartPtr)
        {
            m_pageList->dataSize = m_writePtr - m_writeStartPtr;
            m_writeOffsetSoFar += m_pageList->dataSize;
        }

        // allocate page memory
        auto* page = (Page*)m_allocator->allocatePage();
        page->next = nullptr;
        page->prev = m_pageList;
        page->dataSize = 0;
        page->offsetSoFar = m_writeOffsetSoFar;
        if (m_pageList) m_pageList->next = page;
        m_pageList = page;

        // link to list
        if (nullptr == m_pageHead)
            m_pageHead = page;

        // setup write pointers
        m_writeEndPtr = (uint8_t*)page + m_pageSize;
        m_writeStartPtr = AlignPtr((uint8_t*)page + sizeof(Page), m_elementAlignment);
        m_writePtr = m_writeStartPtr;
        m_elementsLeftOnPage = (m_writeEndPtr - m_writeStartPtr) / m_elementSize;
        m_writeEndPtr = m_writeStartPtr + (m_elementsLeftOnPage * m_elementSize); // make sure end is the last element we can write
    }

    Buffer PagedBufferBase::toBuffer(mem::PoolID pool /*= POOL_DEFAULT*/) const
    {
        if (pool.value() == POOL_DEFAULT)
            pool = m_allocator->poolID();

        auto buf = Buffer::Create(pool, dataSize(), m_elementAlignment);
        if (buf)
            copy(buf.data(), buf.size());

        return buf;
    }

} // base