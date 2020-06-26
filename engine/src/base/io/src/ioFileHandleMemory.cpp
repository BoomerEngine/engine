/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#include "build.h"
#include "ioFileHandleMemory.h"

namespace base
{
    namespace io
    {

        ///--

        MemoryReaderFileHandle::MemoryReaderFileHandle(const void* memory, uint64_t size, const StringBuf& origin /*= StringBuf::EMPTY()*/)
            : m_data((const uint8_t*)memory)
            , m_pos(0)
            , m_size(size)
            , m_origin(origin)
        {}

        MemoryReaderFileHandle::MemoryReaderFileHandle(const Buffer& buffer, const StringBuf& origin /*= StringBuf::EMPTY()*/)
            : m_data(buffer.data())
            , m_size(buffer.size())
            , m_pos(0)
            , m_origin(origin)
            , m_buffer(buffer)
        {}

        MemoryReaderFileHandle::~MemoryReaderFileHandle()
        {}

        const StringBuf& MemoryReaderFileHandle::originInfo() const
        {
            return m_origin;
        }

        uint64_t MemoryReaderFileHandle::size() const
        {
            return m_size;
        }

        uint64_t MemoryReaderFileHandle::pos() const
        {
            return m_pos;
        }

        bool MemoryReaderFileHandle::pos(uint64_t newPosition)
        {
            if (newPosition > m_size)
            {
                TRACE_ERROR("Trying to set read position {} outside memory buffer '{}' size {}", newPosition, m_size, m_origin);
                return false;
            }

            m_pos = newPosition;
            return true;
        }

        bool MemoryReaderFileHandle::isReadingAllowed() const
        {
            return true;
        }

        bool MemoryReaderFileHandle::isWritingAllowed() const
        {
            return false;
        }

        uint64_t MemoryReaderFileHandle::writeSync(const void* data, uint64_t size)
        {
            TRACE_ERROR("Writing is not allowed to memory buffer '{}'", m_origin);
            return 0;
        }

        uint64_t MemoryReaderFileHandle::readSync(void* data, uint64_t size)
        {
            auto maxRead = size;
            if (m_pos + size > m_size)
                maxRead = m_size - m_pos;

            memcpy(data, m_data + m_pos, maxRead);
            m_pos += maxRead;
            return maxRead;
        }

        uint64_t MemoryReaderFileHandle::readAsync(uint64_t offset, uint64_t size, void* readBuffer)
        {
            // offset beyond the range
            if (offset >= m_size)
                return 0;

            // limit the read if close to the end
			auto maxRead = size;
            if (offset + size > m_size)
                maxRead = m_size - offset;

            memcpy(readBuffer, m_data + offset, maxRead);
            return maxRead;
        }

        ///--

    } // io
} // base