/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\memory #]
***/

#include "build.h"
#include "memoryReader.h"

namespace base
{
    namespace stream
    {

        MemoryReader::MemoryReader(const void* data, uint64_t size, uint64_t initialOffset /*= 0*/)
            : IBinaryReader( (uint32_t)BinaryStreamFlags::MemoryBased )
            , m_base((const uint8_t*)data)
        {
            DEBUG_CHECK_EX(initialOffset <= size, "Initial offset outside the buffer");
            m_pos = m_base + initialOffset;
            m_end = m_base + size;
        }

        MemoryReader::MemoryReader(const Buffer& data, uint64_t initialOffset/* = 0*/)
            : IBinaryReader((uint32_t)BinaryStreamFlags::MemoryBased)
            , m_buffer(data)
        {
            if (data)
            {
                m_base = data.data();
                m_pos = m_base + initialOffset;
                m_end = m_base + data.size();
            }
            else
            { 
                m_base = nullptr;
                m_pos = nullptr;
                m_end = nullptr;
            }

            uint64_t size = m_end - m_base;
            DEBUG_CHECK_EX(initialOffset <= size, "Initial offset outside the buffer");
        }

        MemoryReader::~MemoryReader()
        {
        }

        void MemoryReader::read(void* data, uint32_t size)
        {
            if (isError())
                return;

            if (m_pos + size > m_end)
            {
                reportError(TempString("Out of bounds memory file read of {} bytes at offset {}, size {}", size, m_pos - m_base, m_end - m_base));
                memset(data, 0xCC, size);
            }
            else
            {
                memcpy(data, m_pos, size);
                m_pos += size;
            }
        }

        void MemoryReader::read1(void* data)
        {
            memcpy(data, m_pos, 1);
            m_pos += 1;
        }

        void MemoryReader::read2(void* data)
        {
            memcpy(data, m_pos, 2);
            m_pos += 2;
        }

        void MemoryReader::read4(void* data)
        {
            memcpy(data, m_pos, 4);
            m_pos += 4;
        }

        void MemoryReader::read8(void* data)
        {
            memcpy(data, m_pos, 8);
            m_pos += 8;
        }

        void MemoryReader::read16(void* data)
        {
            memcpy(data, m_pos, 16);
            m_pos += 16;
        }

        uint64_t MemoryReader::pos() const
        {
            return m_pos - m_base;
        }

        uint64_t MemoryReader::size() const
        {
            return m_end - m_base;
        }

        void MemoryReader::seek(uint64_t pos)
        {
            m_pos = m_base + pos;

            if (m_pos > m_end)
                reportError(TempString("Seek past buffer size in memory file reader at offset {}, size {}", m_pos - m_base, m_end - m_base));
        }

    } // stream

} // base