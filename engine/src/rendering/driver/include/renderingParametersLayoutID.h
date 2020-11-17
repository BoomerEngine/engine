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
    class ParametersLayoutInfo;

    ///--

    /// Global, unique ID of the parameter layout
    /// This class identifies the layout of the resource entries in the parameter block (descriptor)
    /// Layout itself is the only reasonable way to identify stuff, we don't need a name as there may be many different
    class RENDERING_DRIVER_API ParametersLayoutID
    {
    public:
        INLINE ParametersLayoutID() {}
        INLINE explicit ParametersLayoutID(uint16_t id_) : id(id_) {};

        // is the ID empty (unassigned)
        INLINE bool empty() const { return id == 0; }
        INLINE operator bool() const { return id != 0; }

        // get the numerical value, can be used to index things
        INLINE uint16_t value() const { return id; }

        // compare
        INLINE bool operator==(ParametersLayoutID other) const { return id == other.id; }
        INLINE bool operator!=(ParametersLayoutID other) const { return id != other.id; }

        // get a string representation of the layout, usually something like C-C-IR-IR-IW, etc describing the descriptor layout
        // NOTE: slow  (global lock), for debug only
        void print(base::IFormatStream& f) const;

        // get the layout description
        // NOTE: slow (global lock), don't call to often
        const ParametersLayoutInfo& layout() const;

        // get the memory size required to upload data with this layout to renderer
        uint32_t memorySize() const;

        ///---

        /// register a layout ID form a manual layout
        /// NOTE: this may return existing one if it matches the layout
        static ParametersLayoutID Register(const ParametersLayoutInfo& info);

        /// register a layout ID form a string description, this allows to specify the layouts at compile time
        /// NOTE: this may return existing one if it matches the layout
        /// NOTE: layout format is very simple: a letter for each resource type "I" "B" "C", etc
        static ParametersLayoutID Register(base::StringView layoutDesc);

        /// get a layout from a structure in the memory
        /// NOTE: it must be tightly packed structure of Views (ConstantView, BufferView, ImageView, etc)
        static ParametersLayoutID FromData(const void* data, uint32_t dataSize);

        /// get a layout from a list of object types (ObjectViewType)
        /// NOTE: it must be tightly packed array of ObjectViewType
        static ParametersLayoutID FromObjectTypes(const ObjectViewType* types, uint32_t numTypes);

    private:
        uint16_t id;
    };

    ///--

} // rendering
