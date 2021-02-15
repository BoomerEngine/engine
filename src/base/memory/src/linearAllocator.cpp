/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\pages #]
***/

#include "build.h"
#include "linearAllocator.h"
#include "pageCollection.h"
#include "pageAllocator.h"

namespace base
{
    namespace mem
    {

        //---

        LinearAllocator::LinearAllocator(PoolTag pool, uint32_t tailReserve)
            : m_poolId(pool)
            , m_tailReserve(tailReserve)
        {
            m_pageAllocator = &PageAllocator::GetDefaultAllocator(pool);
            m_pageCollection = PageCollection::CreateFromAllocator(*m_pageAllocator);
            m_ownsPageCollection = true;
        }

        LinearAllocator::LinearAllocator(PageAllocator& pageAllocator, uint32_t tailReserve)
            : m_pageAllocator(&pageAllocator)
        {
            m_pageCollection = PageCollection::CreateFromAllocator(*m_pageAllocator);
            m_ownsPageCollection = true;
        }

        LinearAllocator::LinearAllocator(PageCollection* pageCollection, uint8_t* initialBufferPtr, uint8_t* initialBufferEnd, uint32_t tailReserve)
            : m_pageAllocator(&pageCollection->allocator())
            , m_pageCollection(pageCollection)
            , m_ownsPageCollection(true)
            , m_curBuffer(initialBufferPtr)
            , m_curBufferEnd(initialBufferEnd)
        {
        }

        LinearAllocator::~LinearAllocator()
        {
            if (m_ownsPageCollection)
                m_pageCollection->reset();

            m_pageCollection = nullptr;
            m_pageAllocator = nullptr;
        }

        LinearAllocator* LinearAllocator::CreateFromAllocator(PageAllocator& pageAllocator, uint32_t tailReserve)
        {
            if (auto* pageCollection = PageCollection::CreateFromAllocator(pageAllocator))
            {
                if (auto* mem = (uint8_t*)pageCollection->allocatePage())
                {
                    auto* writeEndPtr = mem + pageCollection->pageSize() - tailReserve;

                    auto* linearAllocatorMem = AlignPtr(mem, alignof(LinearAllocator));
                    auto* writeStartPtr = linearAllocatorMem + sizeof(LinearAllocator);

                    return new (linearAllocatorMem) LinearAllocator(pageCollection, writeStartPtr, writeEndPtr, tailReserve);
                }
            }

            return nullptr; // OOM
        }

        void LinearAllocator::clear()
        {
            DEBUG_CHECK_EX(m_ownsPageCollection, "Cleanup can only be performed on allocators that actually own the page collectio");

            if (!m_ownsPageCollection)
                return;

            m_pageCollection->reset();

            m_pageCollection = PageCollection::CreateFromAllocator(*m_pageAllocator);
            m_ownsPageCollection = true;
        }

        void LinearAllocator::deferCleanup(TCleanupFunc func, void* userData)
        {
            m_pageCollection->deferCleanup(func, userData);
        }

        void* LinearAllocator::alloc(uint64_t size, uint32_t align)
        {
            m_numAllocations += 1;
            m_totalUsedMemory += size;

            auto* ret = AlignPtr(m_curBuffer, align);
            if (ret + size <= m_curBufferEnd)
            {
                m_curBuffer = ret + size;
            }
            else
            {
                m_totalWastedMemory += (m_curBufferEnd - m_curBuffer);

                const auto outstandingAllocSize = (m_pageCollection->pageSize() / 2);
                if (size <= outstandingAllocSize)
                {
                    m_curBuffer = (uint8_t*)m_pageCollection->allocatePage();
                    m_curBufferEnd = m_curBuffer + m_pageCollection->pageSize() - m_tailReserve;

                    ret = AlignPtr(m_curBuffer, align);
                    m_curBuffer += size;
                }
                else
                {
                    ret = (uint8_t*)m_pageCollection->allocateOustandingBlock(size, align);
                }
            }

            return ret;
        }

        //---

    } // mem
} // base
