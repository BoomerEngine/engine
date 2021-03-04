/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\pages #]
***/

#include "build.h"
#include "pageAllocator.h"
#include "pageCollection.h"

#include "core/system/include/scopeLock.h"

BEGIN_BOOMER_NAMESPACE()

//--

PageCollection::PageCollection(PoolTag pool /*= POOL_TEMP*/)
    : m_allocator(PageAllocator::GetDefaultAllocator(pool))
{
    m_pageSize = m_allocator.pageSize();// -sizeof(PageInUse);
}

PageCollection::PageCollection(PageAllocator& pageAllocator)
    : m_allocator(pageAllocator)
{
    m_pageSize = m_allocator.pageSize();// -sizeof(PageInUse);
}

PageCollection* PageCollection::CreateFromAllocator(PageAllocator& pageAllocator)
{
    // allocate a page that will be used to build the collection
    void* page = pageAllocator.allocatePage();
    if (!page)
        return nullptr;

    // build collection in the allocated page
    return new (page) PageCollection(pageAllocator, page);
}

PageCollection::PageCollection(PageAllocator& pageAllocator, void* selfPage)
    : m_allocator(pageAllocator)
{
    m_pageSize = m_allocator.pageSize();
    m_totalSize += m_pageSize;

    m_internalPageCur = (uint8_t*)selfPage + sizeof(PageCollection);
    m_internalPageEnd = (uint8_t*)selfPage + m_pageSize;

    m_selfBlock = selfPage;
}

PageCollection::~PageCollection()
{
    reset();

    DEBUG_CHECK_EX(m_pageList == nullptr, "Pages still allocated");
    DEBUG_CHECK_EX(m_outstandingAllocList == nullptr, "Outstanding blocks still allocated");
}

void PageCollection::reset()
{
    // clear size
    m_totalSize = 0;

    // call cleanup functions
    while (m_cleanupList)
    {
        auto* alloc = m_cleanupList;
        alloc->func(alloc->userData);
        m_cleanupList = alloc->prev;
    }

    // release all outstanding allocations
    while (m_outstandingAllocList)
    {
        auto* alloc = m_outstandingAllocList;
        auto* prev = m_outstandingAllocList->prev;

        if (alloc->freeFunc)
            alloc->freeFunc(alloc->ptr, alloc->size, alloc->freeFuncUserData);

        FreeBlock(alloc->ptr);

        m_outstandingAllocList = prev;
    }

    // release normal pages
    PageInUse* prevPage = nullptr;
    while (m_pageList)
    {
        auto* page = m_pageList;
        prevPage = m_pageList;
        m_pageList = m_pageList->prev;
        m_allocator.freePage(page->page);
    }

    // free the self block if used
    if (m_selfBlock)
    {
        m_allocator.freePage(m_selfBlock);
        // NOTE LEGAL TO TOUCH ANYTHING
    }
}

void PageCollection::deferCleanup(TPageCollectionCleanupFunc func, void* userData)
{
    DEBUG_CHECK(func != nullptr);

    auto* info = (CleanupFunc*)allocateInternal_NoLock(sizeof(CleanupFunc));
    info->func = func;
    info->userData = userData;
    info->prev = m_cleanupList;
    m_cleanupList = info;
}

void* PageCollection::allocatePage()
{
    auto* page = m_allocator.allocatePage();
    if (nullptr == page)
        return nullptr;

    auto lock = CreateLock(m_lock);

    auto* info = (PageInUse*)allocateInternal_NoLock(sizeof(PageInUse));
    info->page = page;
    info->prev = m_pageList;
    m_pageList = info;
    m_totalSize += m_pageSize;

    return page;
}

void* PageCollection::allocateInternal_NoLock(uint32_t size)
{
    auto* ptr = m_internalPageCur;

    if (m_internalPageCur + size <= m_internalPageEnd)
    {
        m_internalPageCur += size;
    }
    else
    {
        auto* page = m_allocator.allocatePage();
        m_internalPageCur = (uint8_t*)page;
        m_internalPageEnd = m_internalPageCur + m_pageSize;
        m_totalSize += m_pageSize;

        auto* header = (PageInUse*)m_internalPageCur;
        header->page = page;
        header->prev = m_pageList;
        m_internalPageCur += sizeof(PageInUse);

        ptr = m_internalPageCur;
        m_internalPageCur += size;
    }

    return ptr;
}

void* PageCollection::allocateOustandingBlock(uint64_t size, uint32_t alignment, TOutstandingMemoryCleanupFunc freeFunc /*= nullptr*/, void* freeFuncUserData /*= nullptr*/)
{
    auto* block = AllocateBlock(m_allocator.poolID(), size, alignment, "OutstandingAlloc");
    if (nullptr == block)
        return nullptr;
            
    {
        auto lock = CreateLock(m_lock);
        auto* alloc = (OutstandingAlloc*)allocateInternal_NoLock(sizeof(OutstandingAlloc));
        alloc->prev = m_outstandingAllocList;
        alloc->size = size;
        alloc->ptr = block;
        alloc->freeFunc = freeFunc;
        alloc->freeFuncUserData = freeFuncUserData;
        m_outstandingAllocList = alloc;
    }

    return block;
}

//---

END_BOOMER_NAMESPACE()
