/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#pragma once

namespace base
{
    /// Very simple pool for structure
    template< typename T >
    class SimpleStructurePool : public base::NoCopy
    {
    public:
        SimpleStructurePool(uint32_t perPage)
            : m_perPage(perPage)
        {
            allocPage();
        }

        void release(T* ptr)
        {
            auto* page = m_pages;
            while (page)
            {
                if (ptr >= page->first && ptr <= page->last)
                {
                    auto* elem = (FreeElem*)ptr;
                    elem->next = page->free;
                    page->free = elem;
                    return;
                }

                page = page->next;
            }

            abort();
        }

        T* alloc()
        {
            // try to alloc from hot page
            auto* startPage = m_hotPage;
            do
            {
                // try to allocate from current page
                if (auto* ret = allocFromCurrentPage())
                    return ret;

                // allocating from current hot page failed, go to next page
                m_hotPage = m_hotPage->next ? m_hotPage->next : m_pages;
            } while (m_hotPage != startPage); // we looped back

            // allocate new page
            allocPage();
            return allocFromCurrentPage();
        }

    private:
        struct FreeElem
        {
            FreeElem* next = nullptr;
        };

        struct Page
        {
            T* first = nullptr;
            T* last = nullptr;
            FreeElem* free = nullptr;
            Page* next = nullptr;
        };

        Page* m_pages = nullptr;
        Page* m_hotPage = nullptr;
        uint32_t m_perPage = 0;

        T* allocFromCurrentPage()
        {
            auto ret = (T*)m_hotPage->free;

            if (m_hotPage->free)
                m_hotPage->free = m_hotPage->free->next;

            return ret;
        }

        void allocPage()
        {
            uint32_t pageSize = (m_perPage * sizeof(T)) + sizeof(Page);
            uint8_t* pageMemory = new uint8_t[pageSize];
            uint8_t* pageMemoryEnd = pageMemory + pageSize;

#ifndef BUILD_RELEASE
            memset(pageMemory, 0xCD, pageSize);
#endif

            auto* page = (Page*)pageMemory;
            page->free = nullptr;
            page->first = AlignPtr((T*)(pageMemory + sizeof(Page)), alignof(T));

            uint32_t bytesLeft = (pageMemoryEnd - (uint8_t*)page->first);
            page->last = page->first + (bytesLeft / sizeof(T));

            if ((uint8_t*)(page->last + 1) > pageMemoryEnd)
                page->last -= 1;

            for (auto* ptr = page->last; ptr >= page->first; --ptr)
            {
                auto* freeElem = (FreeElem*)ptr;
                freeElem->next = page->free;
                page->free = freeElem;
            }

            page->next = m_pages;
            m_pages = page;
            m_hotPage = page;
        }
    };

} // base

