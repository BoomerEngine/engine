/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\native #]
***/

#include "build.h"
#include "nativeFileWriter.h"

namespace base
{
    namespace stream
    {

        NativeFileWriter::NativeFileWriter(const io::FileHandlePtr& filePtr)
            : IBinaryWriter(0)
            , m_file(filePtr)
            , m_pos(filePtr->pos())
            , m_bufferCount(0)
        {
        }

        NativeFileWriter::~NativeFileWriter()
        {
            flush();
        }

        uint64_t NativeFileWriter::pos() const
        {
            return m_pos;
        }

        uint64_t NativeFileWriter::size() const
        {
            return m_file->size();
        }

        void NativeFileWriter::seek(uint64_t pos)
        {
            // Flush pending data
            flush();

            // Set new file position
            if (!m_file->pos(pos))
            {
                reportError(TempString("SetFilePointer failed: {}/{}: {}", pos, size(), this->pos()));
                return;
            }

            // Set file pos
            m_pos = pos;
        }

        void NativeFileWriter::write(const void *data, uint32_t size)
        {
            // Already in error state
            if (isError())
                return;

            // If we have more to write that will fit in current block
            if (size > (CACHE_SIZE - m_bufferCount))
            {
                // Flush what's left to write
                flush();

                // Write block
                auto count = m_file->writeSync(data, size);
                if (count != size)
                {
                    reportError(TempString("WriteFile failed {}/{}", size, count));
                    return;
                }

                // Shift position
                m_pos += count;
                return;
            }

            // Add to current block
            memcpy(m_buffer + m_bufferCount, data, size);
            m_bufferCount += size;
            m_pos += size;
        }

        void NativeFileWriter::flush()
        {
            // Already in error state
            if (isError())
                return;

            // Nothing to write
            if (m_bufferCount)
            {
                // Write data
                auto count = m_file->writeSync(m_buffer, m_bufferCount);
                if (count != m_bufferCount)
                {
                    reportError(TempString("WriteFile failed {}/{}", m_bufferCount, count));
                    return;
                }

                // Reset buffer
                m_bufferCount = 0;
            }
        }

        IBinaryWriter* CreateNativeStreamWriter(const io::FileHandlePtr& filePtr)
        {
            // no file or file not opened for writing
            if (!filePtr || !filePtr->isWritingAllowed())
                return nullptr;

            // create the wrapper
            return MemNew(NativeFileWriter, filePtr);
        }

    } // stream

} // base


