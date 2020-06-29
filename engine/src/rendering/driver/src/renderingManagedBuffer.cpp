/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingManagedBuffer.h"
#include "renderingCommandWriter.h"
#include "renderingDriver.h"

namespace rendering
{

    //---

    ManagedBuffer::ManagedBuffer(const BufferCreationInfo& info)
        : m_creationInfo(info)
    {
        m_creationInfo.allowCopies = true;
        m_creationInfo.allowDynamicUpdate = true;

        m_bufferView = device()->createBuffer(m_creationInfo);

        m_backingStorage = (uint8_t*)MemAlloc(POOL_TEMP, info.size, 16);
        m_backingStorageEnd = m_backingStorage + info.size;

        m_structureGranularity = info.stride;
    }

    ManagedBuffer::~ManagedBuffer()
    {
        MemFree(m_backingStorage);
        m_backingStorage = nullptr;
        m_backingStorageEnd = nullptr;

        m_bufferView.destroy();
    }

    void ManagedBuffer::handleDeviceReset()
    {
        // TODO
    }

    void ManagedBuffer::handleDeviceRelease()
    {
        // TODO
    }

    base::StringBuf ManagedBuffer::describe() const
    {
        return base::TempString("ManagedBuffer {}, {}", MemSize(m_creationInfo.size), m_creationInfo.label);
    }

    void ManagedBuffer::update(command::CommandWriter& cmd)
    {
        auto lock = CreateLock(m_stateLock);
        if (m_dirtyRegionEnd > m_dirtyRegionStart)
        {
            const auto uploadSize = m_dirtyRegionEnd - m_dirtyRegionStart;
            cmd.opUpdateDynamicBuffer(m_bufferView, m_dirtyRegionStart, uploadSize, m_backingStorage + m_dirtyRegionStart);

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

} // rendering