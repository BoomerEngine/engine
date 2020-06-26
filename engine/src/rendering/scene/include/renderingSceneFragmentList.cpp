/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\fragments #]
***/

#include "build.h"
#include "renderingSceneFragmentList.h"

namespace rendering
{
    namespace scene
    {

        //--

        FragmentDrawList::FragmentDrawList(base::mem::LinearAllocator& memory)
            : m_memory(memory)
        {
            m_fragmentsPerPage = 4096;
        }

        FragmentDrawList::~FragmentDrawList()
        {}

        void FragmentDrawList::collectFragment(const Fragment* frag, FragmentDrawBucket bucket)
        {
            auto& list = m_lists[(int)bucket];

            if (!list.tail || list.tail->count == m_fragmentsPerPage)
            {
                auto page = (FragmentPage*)m_memory.alloc(sizeof(FragmentPage) + (m_fragmentsPerPage - 1) * sizeof(Fragment*), 8);
                page->count = 0;
                page->next = nullptr;

                if (list.tail)
                    list.tail->next = page;
                else
                    list.head = page;
                list.tail = page;
            }

            list.tail->fragments[list.tail->count++] = frag;
        }

        void FragmentDrawList::iterateFragmentRanges(FragmentDrawBucket bucket, const std::function<void(const Fragment* const*fragments, uint32_t count)>& enumFunc) const
        {
            const auto* page = m_lists[(int)bucket].head;

            while (page)
            {
                if (page->count)
                    enumFunc(&page->fragments[0], page->count);
                page = page->next;
            }
        }

        //--

    } // scene
} // rendering


