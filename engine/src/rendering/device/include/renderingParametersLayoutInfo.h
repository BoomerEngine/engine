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

#include "base/containers/include/inplaceArray.h"

namespace rendering
{
    class ParametersLayoutInfoBuilder;

    /// info about parameter descriptor
    class RENDERING_DEVICE_API ParametersLayoutInfo
    {
    public:
        ParametersLayoutInfo();
        ParametersLayoutInfo(const ParametersLayoutInfo& other) = default;
        ParametersLayoutInfo(const ParametersLayoutInfo& other, ParametersLayoutID id);
        ParametersLayoutInfo& operator=(const ParametersLayoutInfo& other) = default;

        // is the layout empty ? should not be the case for valid layouts
        INLINE bool empty() const { return m_entries.empty(); }

        // get number of entries in the layout
        INLINE uint32_t size() const { return m_entries.size(); }

        // get n-th entry
        INLINE const ObjectViewType operator[](uint32_t index) const { return m_entries[index]; }

        // get the unique layout hash
        INLINE uint64_t hash() const { return m_hash; }

        // get the memory size needed to copy the data
        INLINE uint32_t memorySize() const { return m_memorySize; }

        // get the ID we are registered under
        INLINE ParametersLayoutID id() const { return m_id; }

        // compare layouts for being equal
        bool operator==(const ParametersLayoutInfo& other) const;

        // get a string representation of the layout, usually something like CCIIB, etc describing the descriptor layout
        void print(base::IFormatStream& f) const;

        // validate memory to make sure it contains compatible data
        // NOTE: asserts on any problems
        bool validateMemory(const void* memoryPtr, uint32_t memorySize) const;

        // validate that all resource bindings are setup correctly
        // NOTE: asserts on any problems
        bool validateBindings(const void* memoryPtr) const;

    private:
        // data hash
        uint64_t m_hash;

        // size of required memory to copy to the command stream
        // NOTE: this is our data size, not the platform dependent one
        uint32_t m_memorySize;

        // the assigned ID
        ParametersLayoutID m_id;

        // layout entries
        base::InplaceArray<ObjectViewType, 32> m_entries;

        //---

        friend class ParametersLayoutID;
    };

} // rendering
