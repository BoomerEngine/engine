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

BEGIN_BOOMER_NAMESPACE()

InstanceBuffer::InstanceBuffer(const InstanceBufferLayout* layout, void* data, uint32_t size, PoolTag poolID)
    : m_layout(AddRef(layout))
    , m_poolID(poolID)
    , m_data(data)
    , m_size(size)
{}

InstanceBuffer::~InstanceBuffer()
{
    m_layout->destroyBuffer(m_data);
    m_layout.reset();

    FreeBlock(m_data);
    m_data = nullptr;
    m_size = 0;
}

InstanceBufferPtr InstanceBuffer::copy() const
{
    void* ptr = AllocateBlock(m_poolID, m_size, m_layout->alignment(), "InstanceBuffer");
    m_layout->copyBufer(ptr, m_data);
    return RefNew<InstanceBuffer>(m_layout, ptr, m_size, m_poolID);
}

void* InstanceBuffer::GetInstanceVarData(const InstanceVarBase& v)
{
    DEBUG_CHECK_SLOW_EX(v.allocated(), "Used InstanceVar that has no allocated space in the buffer");
    return OffsetPtr(m_data, v.offset());
}

const void* InstanceBuffer::GetInstanceVarData(const InstanceVarBase& v) const
{
    DEBUG_CHECK_SLOW_EX(v.allocated(), "Used InstanceVar that has no allocated space in the buffer");
    return OffsetPtr(m_data, v.offset());
}

END_BOOMER_NAMESPACE()
