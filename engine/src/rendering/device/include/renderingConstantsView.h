/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#pragma once

#include "renderingObjectView.h"

namespace rendering
{
    /// view on uploaded constants
    TYPE_ALIGN(4, class) RENDERING_DEVICE_API ConstantsView : public ObjectView
    {
    public:
        ConstantsView();
        ConstantsView(const uint32_t* offsetPtr, uint32_t offset, uint32_t size);

        //----

        INLINE uint32_t offset() const { return m_offset; }
        INLINE const uint32_t* offsetPtr() const { return m_offsetPtr; }
        INLINE uint32_t size() const { return m_size; }

        //---

        INLINE bool empty() const { return m_offset == INVALID_LOCATION; }
        INLINE operator bool() const { return m_offset != INVALID_LOCATION; }

        //--

        /// get a view offset from current view
        /// NOTE: no boundary or range checks
        ConstantsView createOffsetView(uint32_t offset, uint32_t newSize) const;

    private:
        static const uint32_t INVALID_LOCATION = 0xFFFFFFFFU;

        uint32_t m_offset;
        uint32_t m_size;
        const uint32_t* m_offsetPtr;
    };

    static_assert(sizeof(ConstantsView) == 32, "There are places that take assumptions of layout of this structure");

} // rendering
