/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "managedBuffer.h"
#include "commandWriter.h"
#include "device.h"
#include "deviceService.h"
#include "resources.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//---

ManagedBuffer::ManagedBuffer(const BufferCreationInfo& info)
{
    auto creationInfo = info;
    creationInfo.allowCopies = true;
    creationInfo.allowDynamicUpdate = true;

    auto device = GetService<DeviceService>()->device();
    m_bufferObject = device->createBuffer(creationInfo);

    m_backingStorage = GlobalPool<POOL_API_BACKING_STORAGE, uint8_t>::Alloc(info.size, 16);//, info.label.empty() ? "ManagedBuffer" : info.label.c_str());
    m_backingStorageEnd = m_backingStorage + info.size;

    m_structureGranularity = info.stride;
}

ManagedBuffer::~ManagedBuffer()
{
    GlobalPool<POOL_API_BACKING_STORAGE, uint8_t>::Free(m_backingStorage);
    m_backingStorage = nullptr;
    m_backingStorageEnd = nullptr;

    m_bufferObject.reset();
}

void ManagedBuffer::update(CommandWriter& cmd)
{
    auto lock = CreateLock(m_stateLock);
    if (m_dirtyRegionEnd > m_dirtyRegionStart)
    {
        const auto uploadSize = m_dirtyRegionEnd - m_dirtyRegionStart;
        cmd.opUpdateDynamicBuffer(m_bufferObject, m_dirtyRegionStart, uploadSize, m_backingStorage + m_dirtyRegionStart);

        if (uploadSize > 4096)
        {
            TRACE_SPAM("Uploaded {}", MemSize(uploadSize));
        }

        m_dirtyRegionStart = 0;
        m_dirtyRegionEnd = 0;
    }
}

void ManagedBuffer::writeData(uint32_t offset, uint32_t size, const void* data)
{
    DEBUG_CHECK_EX(m_backingStorage + offset + size <= m_backingStorageEnd, "Write outside the buffer's boundary");

    if (m_backingStorage + offset + size <= m_backingStorageEnd)
    {
        memcpy(m_backingStorage + offset, data, size);

        auto lock = CreateLock(m_stateLock);
        if (m_dirtyRegionStart == m_dirtyRegionEnd)
        {
            m_dirtyRegionStart = offset;
            m_dirtyRegionEnd = offset + size;
        }
        else
        {
            m_dirtyRegionStart = std::min<uint32_t>(m_dirtyRegionStart, offset);
            m_dirtyRegionEnd = std::max<uint32_t>(m_dirtyRegionEnd, offset + size);
        }
    }
}

//---

END_BOOMER_NAMESPACE_EX(gpu)
