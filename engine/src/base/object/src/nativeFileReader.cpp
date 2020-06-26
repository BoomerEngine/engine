/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\native #]
***/

#include "build.h"
#include "nativeFileReader.h"
#include "memoryReader.h"

namespace base
{
    namespace stream
    {

        NativeFileReader::NativeFileReader(io::IFileHandle& file, uint64_t nativeOffset /*= 0*/)
            : IBinaryReader(0)
            , m_file(file)
            , m_size(file.size())
            , m_nativeOffset(nativeOffset)
            , m_pos(0)
            , m_bufferBase(0)
            , m_bufferCount(0)
        {
            m_file.pos(nativeOffset);
        }

        NativeFileReader::~NativeFileReader()
        {
        }

        uint64_t NativeFileReader::pos() const
        {
            return m_pos;
        }

        uint64_t NativeFileReader::size() const
        {
            return m_size;
        }

        void NativeFileReader::precache(size_t size)
        {
            // Make sure we are at the end of the read buffer
            DEBUG_CHECK(m_pos == m_bufferBase + m_bufferCount);

            // Calculate needed align (to keep file position in sync with buffer size)
            size_t align = CACHE_SIZE - (m_pos & (CACHE_SIZE - 1));

            // Calculate how much we should read
            m_bufferBase = m_pos;
            m_bufferCount = range_cast<size_t>(std::min<uint64_t>(m_size - m_pos, std::min(size, align)));

            // Read data
            auto count = m_file.readSync(m_buffer, m_bufferCount);

            // Check for errors
            if (count != m_bufferCount)
                reportError(TempString("ReadFile failed: Count={} BufferCount={}", count, m_bufferCount));
        }

        void NativeFileReader::seek(uint64_t pos)
        {
            DEBUG_CHECK_EX(pos <= m_size, "Request seek past file end");

            // Seek is inside precached data block
            if (pos >= m_bufferBase && pos <= (m_bufferBase + m_bufferCount))
            {
                // Just move pointer and hope for the best
                m_pos = pos;
            }
            else
            {
                // Seek, check for errors
                if (!m_file.pos(m_nativeOffset + pos))
                {
                    reportError(TempString("SetFilePointer failed: {}/{}: {}", pos, m_size, m_pos));
                    return;
                }

                // Set position
                m_pos = pos;

                // Reset precache buffer
                m_bufferBase = pos;
                m_bufferCount = 0;
            }
        }

        void NativeFileReader::read(void *data, uint32_t size)
        {
            // We are in error state
            if (isError())
                return;

            // While there is something to read
            while (size > 0)
            {
                // Calculate how much data we can copy from internal buffer
                auto copy = range_cast<uint32_t>(std::min<uint64_t>(size, m_bufferBase + m_bufferCount - m_pos));

                // There's nothing left in the buffer, read remaining data directly from the file
                if (copy == 0)
                {
                    // If remaining data is larger than buffer size read it directly
                    if (size >= CACHE_SIZE)
                    {
                        // Read file
                        auto count = m_file.readSync(data, size);

                        // Whole block read ?
                        if (count != size)
                        {
                            reportError(TempString("ReadFile failed: Count={} BufferCount={}", count, m_bufferCount));
                            return;
                        }

                        //! Move position
                        m_pos += count;
                        m_bufferBase = m_pos;
                        m_bufferCount = 0;
                        return;
                    }

                    // Precache data
                    precache(CACHE_SIZE);

                    // Calculate how much data we can copy from internal buffer
                    copy = range_cast<uint32_t>(std::min<uint64_t>(size, m_bufferBase + m_bufferCount - m_pos));

                    // EOF ?
                    if (copy == 0)
                        reportError(TempString("ReadFile beyond EOF {}+{}/{}", m_pos, size, m_size));

                    // Exit if error
                    if (isError())
                        return;
                }

                // Copy data to buffer
                memcpy(data, m_buffer + (m_pos - m_bufferBase), copy);
                DEBUG_CHECK(m_pos <= m_size);
                DEBUG_CHECK(copy <= size);

                // Move pos
                m_pos += copy;
                size -= copy;
                data = (uint8_t *)data + copy;
            }
        }

    } // stream
} // base
 

