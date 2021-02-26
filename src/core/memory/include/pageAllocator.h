/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\pages #]
***/

#pragma once

#include "core/system/include/mutex.h"
#include "core/system/include/spinLock.h"

BEGIN_BOOMER_NAMESPACE_EX(mem)

/// Specialized allocator of large memory pages
/// All pages are exactly the same size and are allocated from internal page storage and can be recycled very fast
/// NOTE: allocated pages must be returned to the same allocator
/// NOTE: allocator is NOT tracking all it's pages - if you don't return it then it will leak, use PageCollection for tracking
/// NOTE: pages are ALWAYS allocated from system memory, not from allocator!
class CORE_MEMORY_API PageAllocator : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_ALLOCATOR)

public:
    PageAllocator();
    PageAllocator(PoolTag pool, uint32_t pageSize, uint32_t preallocatedPages, uint32_t freePagesToKeep);
    ~PageAllocator(); // asserts if we destroy allocator while there are still pages in use

    //! is this allocator initialized ?
    INLINE bool initialized() const { return m_pageSize != 0; }

    //! get pool ID this page allocator is for
    INLINE PoolTag poolID() const { return m_poolID; }

    //! get size of single page, NOTE: this may be aligned to system page size (usually 4KB)
    INLINE uint32_t pageSize() const { return m_pageSize; }

    //! get number of pages allocated so far
    INLINE uint32_t numPages() const { return m_numPages; }

    ///! get maximum number of pages ever allocated
    INLINE uint32_t maxPages() const { return m_maxPages; }

    //! get number of free pages at the moment
    INLINE uint32_t numFreePages() const { return m_numFreePages; }

    ///! get maximum number of free pages ever seen
    INLINE uint32_t maxFreePages() const { return m_maxFreePages; }

    ///! get number of maximum free pages we want to retain, all additional pages will be freed
    INLINE uint32_t maxFreePagesToRetain() const { return m_maxFreePagesToRetain; }

    //---

    //! initialize page allocator 
    void initialize(PoolTag pool = POOL_TEMP, uint32_t pageSize = 65536, uint32_t preallocatedPages = 0, uint32_t freePagesToKeep = INDEX_MAX); // NOTE: page size will be usually aligned to the minimal system page size, like 4KB

    //---

    //! allocate single page, returns memory pointer to the page
    //! NOTE: the usable page size is always PAGE_SIZE
    void* allocatePage();

    //! free previously allocated page
    //! NOTE: it must be from this container
    void freePage(void* page);

    //---

    //! set the maximum number of pages to retain
    void retainCount(uint32_t retain);

    //! release free pages, keep only the indicated number
    void releaseFreePages(uint32_t retain=0);

    //---

    //! get default allocator for given pool (64KB pages, small retention, good for general purpose stuff)
    static PageAllocator& GetDefaultAllocator(PoolTag pool = POOL_TEMP);

    //--

private:
    struct FreePage
    {
        FreePage* next = nullptr;
    };

    SpinLock m_lock; // allocating a page

    uint32_t m_pageSize = 0;
    FreePage* m_outstandingFreePageList = nullptr;
    FreePage* m_preallocatedFreePageList = nullptr;
            
    PoolTag m_poolID;
    uint32_t m_numPages = 0;
    uint32_t m_maxPages = 0;
    uint32_t m_numFreePages = 0;
    uint32_t m_maxFreePages = 0;

    uint8_t* m_preallocatedMemoryStart = nullptr;
    uint8_t* m_preallocatedMemoryEnd = nullptr;            

    uint32_t m_maxFreePagesToRetain = 0;
};

END_BOOMER_NAMESPACE_EX(mem)
