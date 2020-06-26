/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: instancing #]
***/

#include "build.h"
#include "instanceBuffer.h"
#include "instanceBufferLayout.h"
#include "instanceVar.h"

namespace base
{

    InstanceBuffer::InstanceBuffer(const InstanceBufferLayout* layout, void* data, uint32_t size, mem::PoolID poolID)
        : m_layout(AddRef(layout))
        , m_poolID(poolID)
        , m_data(data)
        , m_size(size)
    {}

    InstanceBuffer::~InstanceBuffer()
    {
        m_layout->destroyBuffer(m_data);
        m_layout.reset();

        MemFree(m_data);
        m_data = nullptr;
        m_size = 0;
    }

    InstanceBufferPtr InstanceBuffer::copy() const
    {
        void* ptr = MemAlloc(m_poolID, m_size, m_layout->alignment());
        m_layout->copyBufer(ptr, m_data);
        return CreateSharedPtr<InstanceBuffer>(m_layout, ptr, m_size, m_poolID);
    }

    void* InstanceBuffer::GetInstanceVarData(const InstanceVarBase& v)
    {
        DEBUG_CHECK_SLOW_EX(v.allocated(), "Used InstanceVar that has no allocated space in the buffer");
        return base::OffsetPtr(m_data, v.offset());
    }

    const void* InstanceBuffer::GetInstanceVarData(const InstanceVarBase& v) const
    {
        DEBUG_CHECK_SLOW_EX(v.allocated(), "Used InstanceVar that has no allocated space in the buffer");
        return base::OffsetPtr(m_data, v.offset());
    }

} // base