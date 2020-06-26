/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#include "build.h"
#include "renderingConstantsView.h"

namespace rendering
{

    //---

    ConstantsView::ConstantsView()
        : ObjectView(ObjectViewType::Constants, ObjectID())
        , m_offset(INVALID_LOCATION)
        , m_offsetPtr(nullptr)
        , m_size(0)
    {}

    ConstantsView::ConstantsView(const uint32_t* offsetPtr, uint32_t offset, uint32_t size)
        : ObjectView(ObjectViewType::Constants, ObjectID())
        , m_offsetPtr(offsetPtr)
        , m_offset(offset)
        , m_size(size)
    {}

    ConstantsView ConstantsView::createOffsetView(uint32_t offset, uint32_t newSize) const
    {
        ASSERT_EX(!empty(), "Cannot create an offset view of an empty view");
        ASSERT(offset + newSize <= m_size);
        return ConstantsView(m_offsetPtr, m_offset + offset, newSize);
    }

    //---

} // rendering