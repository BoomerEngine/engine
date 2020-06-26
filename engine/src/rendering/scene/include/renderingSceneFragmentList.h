/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\fragments #]
***/

#pragma once

#include "renderingSceneFragment.h"
#include "base/memory/include/linearAllocator.h"

namespace rendering
{
    namespace scene
    {

        /// Collected list of rendering fragments
        class RENDERING_SCENE_API FragmentDrawList : public base::NoCopy
        {
        public:
            FragmentDrawList(base::mem::LinearAllocator& memory);
            ~FragmentDrawList();

            ///---

            /// allocate general memory block
            INLINE void* allocMemory(uint32_t size)
            {
                return m_memory.alloc(size, 8);
            }

            /// allocate a fragment and put it in list
            template< typename T >
            INLINE T* allocFragment()
            {
                auto mem = m_memory.alloc(sizeof(T), alignof(T));
                auto ret = new (mem) T();
                ret->type = T::FRAGMENT_TYPE;
                return ret;
            }

            ///--

            /// add fragment to draw list for given pass bit
            void collectFragment(const Fragment* frag, FragmentDrawBucket bucket);

            // enumerate all fragments
            void iterateFragmentRanges(FragmentDrawBucket bucket, const std::function<void(const Fragment *const* fragments, uint32_t count)>& enumFunc) const;

            ///---

        private:
            base::mem::LinearAllocator& m_memory;
            uint32_t m_fragmentsPerPage;

            struct FragmentPage
            {
                uint32_t count = 0;
                FragmentPage* next = nullptr;
                const Fragment* fragments[1];
            };

            struct FragmentList
            {
                FragmentPage* head = nullptr;
                FragmentPage* tail = nullptr;
            };

            FragmentList m_lists[(int)FragmentDrawBucket::MAX];
        };

    } // scene
} // rendering

