/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#pragma once

#include "renderingObject.h"

namespace rendering
{
    /// type of the object view
    enum class ObjectViewType : uint8_t
    {
        Invalid = 0,
        Constants,
        Buffer,
        Image,
    };

    /// generic form of a view of an object managed by the rendering
    /// NOTE: the object may not yet exist while the view is created
    /// by default the object view takes ownership of the objectID by incrementing the ID
    TYPE_ALIGN(4, class) RENDERING_DEVICE_API ObjectView
    {
    public:
        ObjectView(ObjectViewType type = ObjectViewType::Invalid); // empty
        INLINE ObjectView(const ObjectView& other) = default;;
        INLINE ObjectView(ObjectView&& other) = default;;
        INLINE ObjectView& operator=(const ObjectView& other) = default;
        INLINE ObjectView& operator=(ObjectView&& other) = default;

        INLINE ObjectView(ObjectViewType type, const ObjectID& id)
            : m_id(id)
            , m_type(type)
        {}

        INLINE bool empty() const { return m_id.empty(); }
        INLINE operator bool() const { return !empty(); }

        INLINE ObjectID id() const { return m_id; }
        INLINE ObjectViewType type() const { return m_type; }

        INLINE bool operator==(const ObjectView& other) const { return m_id == other.m_id; }
        INLINE bool operator!=(const ObjectView& other) const { return m_id != other.m_id; }

        //-

        // debug print
        void print(base::IFormatStream& f) const;

        //--

    private:
        ObjectViewType m_type; // NOTE: must be first
        ObjectID m_id;
    };

} // rendering
