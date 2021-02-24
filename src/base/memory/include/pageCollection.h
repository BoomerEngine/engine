/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\pages #]
***/

#pragma once

#include "base/system/include/spinLock.h"

BEGIN_BOOMER_NAMESPACE(base::mem)

typedef void (*TPageCollectionCleanupFunc)(void* userData);
typedef void (*TOutstandingMemoryCleanupFunc)(void* ptr, uint64_t size, void* userData);

/// Collection of pages allocated from a page allocator
/// Pages are returned to the page allocator automatically when this object is destroyed
/// We also support allocations larger than the page size by creating them "off site" and releasing at the same time as the normal pages
class BASE_MEMORY_API PageCollection : public base::NoCopy
{
public:
    PageCollection(PoolTag pool = POOL_TEMP); // use default allocator for given pool
    PageCollection(PageAllocator& pageAllocator); // use specific page allocator
    ~PageCollection();

    /// get the page allocator used by the collection
    INLINE PageAllocator& allocator() const { return m_allocator; }

    /// page size (as in parent page allocator)
    INLINE uint32_t pageSize() const { return m_pageSize; }

    /// total memory allocated in pages and outstanding allocations
    INLINE uint64_t allocatedSize() const { return m_totalSize; }

    //--

    // release all pages and outstanding allocations back to the page allocator
    void reset();

    // allocate a page
    void* allocatePage();

    // allocate outstanding memory, we can provide optional cleanup function to call (NOTE: memory is freed be us, do not free it again in cleanup)
    // NOTE: this is really meant for blocks bigger than page size, not for small blocks
    void* allocateOustandingBlock(uint64_t size, uint32_t alignment = 4, TOutstandingMemoryCleanupFunc freeFunc = nullptr, void* freeFuncUserData = nullptr);

    // inject a cleanup call to call when we free memory
    void deferCleanup(TPageCollectionCleanupFunc func, void* userData);

    //--

    // self-initialize a page collection (allocate a page from page allocator and build page collection in first bytes)
    static PageCollection* CreateFromAllocator(PageAllocator& pageAllocator);

    //--

private:
    PageAllocator& m_allocator;

    PageCollection(PageAllocator& pageAllocator, void* selfPage);

    struct PageInUse
    {
        void* page = nullptr;
        PageInUse* prev = nullptr;
    };

    struct CleanupFunc
    {
        CleanupFunc* prev = nullptr;
        TPageCollectionCleanupFunc func = nullptr;
        void* userData = nullptr;
    };

    struct OutstandingAlloc
    {
        OutstandingAlloc* prev = nullptr;
        void* ptr = nullptr;
        uint64_t size = 0;
        TOutstandingMemoryCleanupFunc freeFunc = nullptr;
        void* freeFuncUserData = nullptr;
    };

    SpinLock m_lock; // allocating a page

    uint32_t m_pageSize = 0;
    uint64_t m_totalSize = 0;

    uint8_t* m_internalPageCur = nullptr;
    uint8_t* m_internalPageEnd = nullptr;

    void* allocateInternal_NoLock(uint32_t size);

    OutstandingAlloc* m_outstandingAllocList = nullptr;
    CleanupFunc* m_cleanupList = nullptr;

    PageInUse* m_pageList = nullptr;

    void* m_selfBlock = nullptr;
};

END_BOOMER_NAMESPACE(base::memory)