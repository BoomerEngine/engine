/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#include "build.h"
#include "renderingBufferView.h"
#include "base/containers/include/stringBuilder.h"

namespace rendering
{
    //---

    BufferView::BufferView(ObjectID id, uint32_t offset, uint32_t size, uint16_t stride, BufferViewFlags flags)
        : ObjectView(ObjectViewType::Buffer, id)
        , m_offset(offset)
        , m_size(size)
        , m_stride(stride)
        , m_flags(flags)
    {
        DEBUG_CHECK_EX(!id.empty(), "Invalid objects can't be used to create pseudo-valid looking views");
        DEBUG_CHECK_EX(!stride || (0 == (m_size % stride)), "Buffer size must be multiple of stride");

        if (id.isTransient())
            m_flags |= BufferViewFlag::Transient;
        if (stride)
            m_flags |= BufferViewFlag::Structured;

        DEBUG_CHECK_EX(!m_flags.test(BufferViewFlag::Structured) || stride , "Non-structured buffers can't have the Structured flag set");
    }

    void BufferView::print(base::IFormatStream& f) const
    {
        f << id();

        if (!empty())
        {
            f.appendf(" {}", m_size);

            if (m_offset != 0)
                f.appendf(" @{}", m_offset);

            if (m_flags.test(BufferViewFlag::Constants)) f << ", CONSTANTS";
            if (m_flags.test(BufferViewFlag::Vertex)) f << ", VERTEX";
            if (m_flags.test(BufferViewFlag::Index)) f << ", INDEX";
            if (m_flags.test(BufferViewFlag::ShaderReadable)) f << ", SHADER";
            if (m_flags.test(BufferViewFlag::UAVCapable)) f << ", UAV";
            if (m_flags.test(BufferViewFlag::Structured)) f << ", STRUCTURED";
            if (m_flags.test(BufferViewFlag::CopyCapable)) f << ", COPY";
            if (m_flags.test(BufferViewFlag::IndirectArgs)) f << ", INDIRECT";
            if (m_flags.test(BufferViewFlag::Dynamic)) f << ", DYNAMIC";
        }
    }

    BufferView BufferView::createSubViewAtOffset(uint32_t offset, uint32_t limitSize /*= 0*/) const
    {
        DEBUG_CHECK_EX(offset < m_size, "Trying to create a view past the buffor size");
        DEBUG_CHECK_EX(!m_stride || (0 == (offset % m_stride)), "Offset must be multiple of structure stride");
        auto maxRemainingSize = size() - offset;
        auto remainingSize = limitSize ? std::min<uint32_t>(limitSize, maxRemainingSize) : maxRemainingSize;
        return BufferView(id(), m_offset + offset, remainingSize, m_stride, flags());
    }

    //---

    TransientBufferView::TransientBufferView(BufferViewFlags type, uint32_t size, uint32_t elementStride /*= 0*/, bool copiable /*= false*/)
        : BufferView(ObjectID::AllocTransientID(), 0, size, elementStride, type | BufferViewFlag::Transient | (copiable ? BufferViewFlag::CopyCapable : BufferViewFlag::None))
    {}

    static BufferViewFlags TranslateTransientUsageFlags(BufferViewFlag primaryType, TransientBufferAccess access)
    {
        DEBUG_CHECK_EX(primaryType == BufferViewFlag::Vertex || primaryType == BufferViewFlag::Index || primaryType == BufferViewFlag::IndirectArgs || 
            primaryType == BufferViewFlag::ShaderReadable || primaryType == BufferViewFlag::Constants || primaryType == BufferViewFlag::IndirectArgs, "Invalid primary type of buffer");

        BufferViewFlags flags = primaryType;

        if (access == TransientBufferAccess::ShaderReadOnly)
        {
            flags |= BufferViewFlag::ShaderReadable;
            flags |= BufferViewFlag::UAVCapable;
        }
        else if (access == TransientBufferAccess::ShaderReadWrite)
        {
            flags |= BufferViewFlag::ShaderReadable;
            flags |= BufferViewFlag::UAVCapable;
        }

        flags |= BufferViewFlag::Transient;

        return flags;
    }

    static BufferViewFlags TranslateTransientUsageFlags(BufferViewFlag primaryType)
    {
        DEBUG_CHECK_EX(primaryType == BufferViewFlag::Vertex || primaryType == BufferViewFlag::Index || primaryType == BufferViewFlag::IndirectArgs ||
            primaryType == BufferViewFlag::ShaderReadable || primaryType == BufferViewFlag::Constants || primaryType == BufferViewFlag::IndirectArgs, "Invalid primary type of buffer");

        BufferViewFlags flags = primaryType;
        flags |= BufferViewFlag::Transient;
        return flags;
    }

    TransientBufferView::TransientBufferView(BufferViewFlag primaryType, TransientBufferAccess access, uint32_t size, uint32_t elementStride /*= 0*/, bool copiable /*= false*/)
        : BufferView(ObjectID::AllocTransientID(), 0, size, elementStride, TranslateTransientUsageFlags(primaryType, access) | (copiable ? BufferViewFlag::CopyCapable : BufferViewFlag::None))
    {}

    TransientBufferView::TransientBufferView(BufferViewFlag primaryType, uint32_t size, uint32_t elementStride, bool copiable /*= false*/)
        : BufferView(ObjectID::AllocTransientID(), 0, size, elementStride, TranslateTransientUsageFlags(primaryType, TransientBufferAccess::NoShaders) | (copiable ? BufferViewFlag::CopyCapable : BufferViewFlag::None))
    {}

    //---

} // rendering