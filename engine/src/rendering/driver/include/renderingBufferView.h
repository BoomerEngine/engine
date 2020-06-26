/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#pragma once

#include "renderingObject.h"
#include "renderingObjectView.h"
#include "renderingResources.h"

namespace rendering
{

    ///---

    /// generic buffer view
    TYPE_ALIGN(4, class) RENDERING_DRIVER_API BufferView : public ObjectView
    {
    public:
        INLINE BufferView() {}
        INLINE BufferView(const BufferView& other) = default;
        INLINE BufferView& operator=(const BufferView& other) = default;
        BufferView(ObjectID id, uint32_t offset, uint32_t size, uint16_t stride, BufferViewFlags flags);

        /// get the offset in buffer the view is created at 
        INLINE uint32_t offset() const { return m_offset; }

        /// get size of the view region (may be zero to indicate the full buffer)
        INLINE uint32_t size() const { return m_size; }

        // structure stride (only structured buffer)
        INLINE uint16_t stride() const { return m_stride; }

        /// get flags
        INLINE BufferViewFlags flags() const { return m_flags; }

        //---

        INLINE bool constants() const { return m_flags.test(BufferViewFlag::Constants); }
        INLINE bool dynamic() const { return m_flags.test(BufferViewFlag::Dynamic); }
        INLINE bool index() const { return m_flags.test(BufferViewFlag::Index); }
        INLINE bool vertex() const { return m_flags.test(BufferViewFlag::Vertex); }
        INLINE bool copyCapable() const { return m_flags.test(BufferViewFlag::CopyCapable); }
        INLINE bool shadeReadable() const { return m_flags.test(BufferViewFlag::ShaderReadable); }
        INLINE bool indirectArgs() const { return m_flags.test(BufferViewFlag::IndirectArgs); }
        INLINE bool structured() const { return m_flags.test(BufferViewFlag::Structured); }
        INLINE bool transient() const { return m_flags.test(BufferViewFlag::Transient); }
        INLINE bool uavCapable() const { return m_flags.test(BufferViewFlag::UAVCapable); }

        ///--

        /// create a sub view of this buffer at a given offset, potentially with partial size
        BufferView createSubViewAtOffset(uint32_t offset, uint32_t limitSize = 0) const;

        ///--

        /// compare view - compares all things - offset/size/stride and object ID
        INLINE bool operator==(const BufferView& other) const { return (id() == other.id()) && (m_offset == other.m_offset) && (m_size == other.m_size) && (m_stride == other.m_stride) && (m_flags == other.m_flags); }
        INLINE bool operator!=(const BufferView& other) const { return !operator==(other); }

        ///--

        void print(base::IFormatStream& f) const;

    private:
        uint32_t m_offset = 0;
        uint32_t m_size = 0;
        uint16_t m_stride = 0;
        BufferViewFlags m_flags;
    };

    static_assert(sizeof(BufferView) == 32, "There are places that take assumptions of layout of this structure");

    ///---

    /// simple transient buffer enum to indicate if we can read/write this buffer in shaders
    enum class TransientBufferAccess : uint8_t
    {
        NoShaders, // buffer is not accessible in shaders, strict geometry/constants
        ShaderReadOnly, // buffer will be read by shaders
        ShaderReadWrite, // buffer will be read and written by shaders
    };

    /// Transient (short lived) buffer that can be used as source and target of various stages
    /// Transient buffers, especially allocating memory for them is managed automatically by the system
    class RENDERING_DRIVER_API TransientBufferView : public BufferView
    {
    public:
        INLINE TransientBufferView() {}
        INLINE TransientBufferView(const TransientBufferView& other) = default;
        INLINE TransientBufferView& operator=(const TransientBufferView& other) = default;
        TransientBufferView(BufferViewFlags type, uint32_t size, uint32_t elementStride = 0, bool copiable=false);
        TransientBufferView(BufferViewFlag primaryType, uint32_t size, uint32_t elementStride = 0, bool copiable = false); // assumes NO UAV access
        TransientBufferView(BufferViewFlag primaryType, TransientBufferAccess access, uint32_t size, uint32_t elementStride = 0, bool copiable = false);
    };

    ///---

} // rendering