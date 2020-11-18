/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#pragma once

#include "renderingObjectView.h"
#include "renderingParametersLayoutID.h"

namespace rendering
{
    ///--

    /// view on uploaded parameters
    class RENDERING_DEVICE_API ParametersView
    {
    public:
        INLINE ParametersView() {}
        INLINE ParametersView(const void* dataPtr, ParametersLayoutID layout, uint32_t index)
            : m_dataPtr(dataPtr)
            , m_layout(layout)
            , m_index(index)
        {}

        //--

        INLINE bool empty() const { return m_dataPtr == nullptr; }
        INLINE operator bool() const { return m_dataPtr != nullptr; }

        INLINE uint32_t index() const { return m_index; }

        INLINE ParametersLayoutID layout() const { return m_layout; }
        INLINE const uint8_t* dataPtr() const { return (const uint8_t*) m_dataPtr; }

        //--

        INLINE bool operator==(const ParametersView& view) const { return m_dataPtr == view.m_dataPtr; }
        INLINE bool operator!=(const ParametersView& view) const { return m_dataPtr != view.m_dataPtr; }

        //--

        void print(base::IFormatStream& f) const;

    private:
        ParametersLayoutID m_layout;
        uint32_t m_index = 0;
        const void* m_dataPtr = nullptr;
    };

    ///--

} // rendering
