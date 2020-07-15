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

        MemoryReaderFileHandle::MemoryReaderFileHandle(const void* memory, uint64_t size)
            : m_data((const uint8_t*)memory)
            , m_pos(0)
            , m_size(size)
        {}

        MemoryReaderFileHandle::MemoryReaderFileHandle(const Buffer& buffer)
            : m_data(buffer.data())
            , m_size(buffer.size())
            , m_pos(0)
            , m_buffer(buffer)
        {}

        MemoryReaderFileHandle::~MemoryReaderFileHandle()
        {}

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
                TRACE_ERROR("MemIO: Trying to set read position {} outside memory buffer of size {}", newPosition, m_size);
                return false;
            }

            m_pos = newPosition;
            return true;
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

        ///--

        MemoryAsyncReaderFileHandle::MemoryAsyncReaderFileHandle(const void* memory, uint64_t size)
            : m_data((const uint8_t*)memory)
            , m_size(size)
        {}

        MemoryAsyncReaderFileHandle::MemoryAsyncReaderFileHandle(const Buffer& buffer)
            : m_buffer(buffer)
            , m_data(buffer.data())
            , m_size(buffer.size())
        {
        }

        MemoryAsyncReaderFileHandle::~MemoryAsyncReaderFileHandle()
        {}

        uint64_t MemoryAsyncReaderFileHandle::size() const
        {
            return m_size;
        }

        uint64_t MemoryAsyncReaderFileHandle::readAsync(uint64_t offset, uint64_t size, void* readBuffer)
        {
            if (offset + size <= m_size)
            {
                memcpy(readBuffer, m_data + offset, size);
                return size;
            }

            if (offset >= m_size)
                return 0;

            auto readSize = std::min<uint64_t>(m_size - offset, size);
            memcpy(readBuffer, m_data + offset, readSize);
            return readSize;
        }

        ///--

        const uint64_t MIN_CAPACITY = 2U << 20;

        MemoryWriterFileHandle::MemoryWriterFileHandle(uint64_t initialSize /*= 1U << 20*/)
        {
            if (initialSize > 0)
            {
                initialSize = std::max<uint64_t>(MIN_CAPACITY, initialSize);
                m_base = (uint8_t*)mem::AAllocSystemMemory(initialSize, true);
                if (m_base)
                {
                    m_pos = m_base;
                    m_end = m_base + initialSize;
                    m_max = m_base;
                }
                else
                {
                    TRACE_WARNING("MemIO: Failed to allocate initial buffer of size {} for memory writer", MemSize(initialSize));
                    m_discarded = true;
                }
            }
        }

        MemoryWriterFileHandle::~MemoryWriterFileHandle()
        {
            freeBuffer();
        }

        static void BufferFreeFunc(mem::PoolID pool, void* memory, uint64_t size)
        {
            mem::AFreeSystemMemory(memory, size);
        }

        Buffer MemoryWriterFileHandle::extract()
        {
            if (m_discarded || !m_base)
                return Buffer();

            const auto size = m_max - m_base;
            auto ret = Buffer::CreateExternal(POOL_IO, size, m_base, &BufferFreeFunc);

            m_base = nullptr;
            m_pos = nullptr;
            m_end = nullptr;
            m_max = nullptr;
            m_discarded = false;

            return ret;
        }

        void MemoryWriterFileHandle::freeBuffer()
        {
            if (m_base)
            {
                const auto size = m_end - m_base;
                mem::AFreeSystemMemory(m_base, size);

                m_base = nullptr;
                m_pos = nullptr;
                m_end = nullptr;
                m_max = nullptr;
            }
        }

        uint64_t MemoryWriterFileHandle::size() const
        {
            return m_max - m_base;
        }

        uint64_t MemoryWriterFileHandle::pos() const
        {
            return m_pos - m_base;
        }

        bool MemoryWriterFileHandle::pos(uint64_t newPosition)
        {
            const auto newPos = m_base + newPosition;
            if (newPos <= m_max)
            {
                m_pos = newPos;
                return true;
            }

            TRACE_WARNING("MemIO: Unable to seek past file's end {} > {}", newPosition, size());
            return false;
        }

        bool MemoryWriterFileHandle::increaseCapacity(uint64_t minAdditionalSize)
        {
            auto currentPos = pos();
            auto currentSize = size();

            // calculate new capacity
            auto requiredSize = currentPos + minAdditionalSize;
            auto capcity = std::max<uint64_t>(m_max - m_base, MIN_CAPACITY);
            while (capcity < requiredSize)
                capcity *= 2;

            // allocate new data
            void* newBuffer = mem::AAllocSystemMemory(capcity, true);
            if (!newBuffer)
            {
                TRACE_WARNING("MemIO: Out of memory when resizing {}->{}", MemSize(size()), MemSize(capcity));
                return false;
            }

            // copy existing data
            memcpy(newBuffer, m_base, size());

            // release old data
            mem::AFreeSystemMemory(m_base, m_end - m_base);

            // setup new buffer
            m_base = (uint8_t*)newBuffer;
            m_pos = m_base + currentPos;
            m_end = m_base + capcity;
            m_max = m_base + currentSize;
            return true;
        }

        uint64_t MemoryWriterFileHandle::writeSync(const void* data, uint64_t size)
        {
            // write all
            if (m_pos + size <= m_end)
            {
                memcpy(m_pos, data, size);
                m_pos += size;
                m_max = std::max(m_max, m_pos);
                return size;
            }

            // write what we can
            uint64_t written = m_end - m_pos;
            {
                ASSERT_EX(written < size, "There was space");

                memcpy(m_pos, data, written);
                m_pos += written;
                m_max = std::max(m_max, m_pos);

                size -= written;
                data = (const uint8_t*)data + written;
            }

            // resize and if successful write the rest
            if (increaseCapacity(size))
            {
                ASSERT_EX(m_pos + size <= m_end, "Resize didn't work");
                memcpy(m_pos, data, size);
                m_pos += size;
                m_max = std::max(m_max, m_pos);
                written += size;
            }

            // return actual number of bytes written
            return written;
        }

        void MemoryWriterFileHandle::discardContent()
        {
            if (!m_discarded)
            {
                TRACE_WARNING("MemIO: File content discarded ({} bytes written so far)", size());
                m_discarded = true;
                freeBuffer();
            }
        }

    } // io
} // base