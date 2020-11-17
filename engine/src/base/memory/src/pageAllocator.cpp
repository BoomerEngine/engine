/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\pages #]
***/

#include "build.h"
#include "pageAllocator.h"
#include "poolStatsInternal.h"

#include "base/system/include/scopeLock.h"

//#define PROTECT_FREE_PAGES

#ifdef PROTECT_FREE_PAGES
#include <Windows.h>
#endif

namespace base
{
    namespace mem
    {

        //--

        PageAllocator::PageAllocator()
        {
        }

        PageAllocator::PageAllocator(PoolTag pool, uint32_t pageSize, uint32_t preallocatedPages, uint32_t freePagesToKeep)
        {
            initialize(pool, pageSize, preallocatedPages, freePagesToKeep);
        }

        PageAllocator::~PageAllocator()
        {
            ASSERT_EX(m_numPages == 0, "Releasing page allocator with pages still allocated from it :(");

            uint64_t totalFreedMemory = 0;

            if (m_preallocatedMemoryStart)
            {
                const auto preallocatedMemorySize = (m_preallocatedMemoryEnd - m_preallocatedMemoryStart);
                FreeSystemMemory(m_preallocatedMemoryStart, preallocatedMemorySize);

                totalFreedMemory += preallocatedMemorySize;
            }

            while (m_outstandingFreePageList)
            {
                auto* next = m_outstandingFreePageList;
                FreeSystemMemory(m_outstandingFreePageList, m_pageSize);
                m_outstandingFreePageList = next;
            }

            prv::TheInternalPoolStats.notifyFree(m_poolID, totalFreedMemory);
        }

        void PageAllocator::initialize(PoolTag pool /*= POOL_TEMP*/, uint32_t pageSize /*= 65536*/, uint32_t preallocatedPages /*= 0*/, uint32_t freePagesToKeep /*= INDEX_MAX*/)
        {
            ASSERT_EX(m_pageSize == 0, "Page allocator already initialized");

            // align page size to system size
            // TODO: get system page size
            const auto systemPageSize = 4096;
            m_pageSize = Align<uint32_t>(pageSize, systemPageSize);
            m_maxFreePagesToRetain = freePagesToKeep;
            m_poolID = pool;

            // preallocate some memory for free pages
            // NOTE: those pages will never be released
            if (preallocatedPages)
            {
                const auto memorySize = (uint64_t)m_pageSize * (uint64_t)preallocatedPages; // we may have > 4GB in pages...
                const auto largePages = false;// memorySize >= (256ULL << 20); // arbitrary choice..
                m_preallocatedMemoryStart = (uint8_t*)AllocSystemMemory(memorySize, largePages);
                m_preallocatedMemoryEnd = m_preallocatedMemoryStart + memorySize;

#ifdef PROTECT_FREE_PAGES
                memset(m_preallocatedMemoryStart, 0xEE, memorySize);
#endif
                prv::TheInternalPoolStats.notifyAllocation(m_poolID, memorySize);

                // create free pages
                auto* pagePtr = m_preallocatedMemoryEnd - m_pageSize;
                while (pagePtr >= m_preallocatedMemoryStart)
                {
                    auto* freePage = (FreePage*)pagePtr;
                    DEBUG_CHECK((uint64_t)freePage != 0x10000);

                    DEBUG_CHECK((uint64_t)m_preallocatedFreePageList != 0x10000);
                    freePage->next = m_preallocatedFreePageList;
                    m_preallocatedFreePageList = freePage;

                    pagePtr -= m_pageSize;
                }

                m_numFreePages = preallocatedPages;
                m_maxFreePages = preallocatedPages;

                DEBUG_CHECK_EX((uint8_t*)m_preallocatedFreePageList == m_preallocatedMemoryStart, "Free list building bug");

#ifdef PROTECT_FREE_PAGES
                DWORD oldProtect;
                VirtualProtect(m_preallocatedMemoryStart, memorySize, PAGE_NOACCESS, &oldProtect);
#endif
            }
        }

        void* PageAllocator::allocatePage()
        {
            // try to allocate from free list
            {
                auto lock = CreateLock(m_lock);

                // use a free page if we can, preffer preallocated pool as it's not going away
                if (auto* page = m_preallocatedFreePageList)
                {
                    DEBUG_CHECK(m_numFreePages > 0);
                    m_numFreePages -= 1;
                    m_numPages += 1; // if we have a page from a free list we can't use more memory than max

#ifdef PROTECT_FREE_PAGES
                    DWORD oldProtect;
                    VirtualProtect(page, m_pageSize, PAGE_READWRITE, &oldProtect);
#endif

                    m_preallocatedFreePageList = m_preallocatedFreePageList->next;
#ifdef PROTECT_FREE_PAGES
                    memset(page, 0xCC, m_pageSize);
#endif

                    return page;
                }
                else if (auto* page = m_outstandingFreePageList)
                {
                    DEBUG_CHECK(m_numFreePages > 0);
                    m_numFreePages -= 1;
                    m_numPages += 1; // if we have a page from a free list we can't use more memory than max

#ifdef PROTECT_FREE_PAGES
                    DWORD oldProtect;
                    VirtualProtect(page, m_pageSize, PAGE_READWRITE, &oldProtect);
#endif
                    m_outstandingFreePageList = m_outstandingFreePageList->next;
#ifdef PROTECT_FREE_PAGES
                    memset(page, 0xDD, m_pageSize);
#endif
                    return page;
                }

                // we will allocate a new page
                m_numPages += 1;
                m_maxPages = std::max<uint32_t>(m_maxPages, m_numPages);
            }

            // no pages in free list, allocate a new one from system memory
            void* systemPage = AllocSystemMemory(m_pageSize, false);
            DEBUG_CHECK_EX(nullptr != systemPage, "Out of memory");
            if (nullptr == systemPage)
            {
                // revert stats, should not matter since we will crash soon :P
                {
                    auto lock = CreateLock(m_lock);
                    m_numPages -= 1;
                    TRACE_ERROR("Out of memory when allocating page in page allocator, pool {}, num pages: {}, page size: {}", (int)m_poolID, m_numPages, m_pageSize);
                }

                return nullptr;
            }

            return systemPage;
        }

        void PageAllocator::freePage(void* page)
        {
            DEBUG_CHECK_EX(page != nullptr, "Trying to free null page");

            // if page is NOT part of initial preallocation consider freeing it
            bool preallocatedPage = false;
            if ((uint8_t*)page >= m_preallocatedMemoryStart && (uint8_t*)page < m_preallocatedMemoryEnd)
                preallocatedPage = true;

            // decide if we should free the page completely or just put it in the free list
            bool releasePageToSystem = false;
            {
                auto lock = CreateLock(m_lock);
                DEBUG_CHECK(m_numPages > 0);
                //DEBUG_CHECK(m_numFreePages <= m_maxFreePagesToRetain);

                m_numPages -= 1;

#ifdef PROTECT_FREE_PAGES
                memset(page, 0xAA, m_pageSize);
#endif
                if (preallocatedPage)
                {
                    auto* freePage = (FreePage*)page;
                    freePage->next = m_preallocatedFreePageList;
                    m_preallocatedFreePageList = freePage;

#ifdef PROTECT_FREE_PAGES
                    DWORD oldProtect;
                    VirtualProtect(freePage, m_pageSize, PAGE_NOACCESS, &oldProtect);
#endif
                    m_numFreePages += 1;
                    m_maxFreePages = std::max<uint32_t>(m_maxFreePages, m_numFreePages);
                }
                else if (m_numFreePages < m_maxFreePagesToRetain)
                {
                    auto* freePage = (FreePage*)page;
                    freePage->next = m_outstandingFreePageList;
                    m_outstandingFreePageList = freePage;

#ifdef PROTECT_FREE_PAGES
                    DWORD oldProtect;
                    VirtualProtect(freePage, m_pageSize, PAGE_NOACCESS, &oldProtect);
#endif
                    m_numFreePages += 1;
                    m_maxFreePages = std::max<uint32_t>(m_maxFreePages, m_numFreePages);
                }
                else
                {
                    releasePageToSystem = true;
                }
            }

            // release the page completely
            if (releasePageToSystem)
                FreeSystemMemory(page, m_pageSize);
        }

        void PageAllocator::retainCount(uint32_t retain)
        {
            {
                auto lock = CreateLock(m_lock);
                m_maxFreePagesToRetain = retain;
            }

            releaseFreePages(retain);
        }

        void PageAllocator::releaseFreePages(uint32_t retain /*= 0*/)
        {
            FreePage* pageListToFree = nullptr;

            // unlink pages we want to free
            {
                auto lock = CreateLock(m_lock);
                auto* page = m_outstandingFreePageList;
                while (m_numFreePages > retain && page)
                {
                    auto* next = page->next;
                    page->next = pageListToFree;
                    pageListToFree = page;
                    m_numFreePages -= 1;

                    page = next;
                }
            }

            // free pages to system
            while (pageListToFree)
            {
                auto* mem = pageListToFree;
                pageListToFree = pageListToFree->next;
                FreeSystemMemory(mem, m_pageSize);
            }
        }

        //---

        namespace helper
        {
            class DefaultPageAllocators : public ISingleton
            {
                DECLARE_SINGLETON(DefaultPageAllocators);

            public:
                PageAllocator& getAllocataor(PoolTag pool)
                {
                    auto lock = CreateLock(m_lock);

                    auto& allocator = m_allocators[pool];
                    if (!allocator.initialized())
                    {
                        uint32_t pageSize = 64 * 1024;
                        uint32_t preallocatedPages = 0;
                        uint32_t freePagesToKeep = 64;

                        if (pool == POOL_TEMP)
                            preallocatedPages = 256;  // 16 MB of temp pages

                        allocator.initialize(pool, pageSize, preallocatedPages, freePagesToKeep);
                    }

                    return allocator;
                }

            private:
                PageAllocator m_allocators[256];
                SpinLock m_lock;

                virtual void deinit() override
                {

                }
            };

        } // helper

        PageAllocator& PageAllocator::GetDefaultAllocator(PoolTag pool /*= POOL_TEMP*/)
        {
            return helper::DefaultPageAllocators::GetInstance().getAllocataor(pool);
        }

        PageAllocator& DefaultPageAllocator(PoolTag pool /*= POOL_TEMP*/)
        {
            return helper::DefaultPageAllocators::GetInstance().getAllocataor(pool);
        }

        ///---

    } // mem
} // base