/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "ioFileHandlePOSIX.h"
#include "ioAsyncDispatcherPOSIX.h"

namespace base
{
    namespace io
    {
        namespace prv
        {

            POSIXFileHandle::POSIXFileHandle(int hFile, const StringBuf& origin, bool reader, bool writer, POSIXAsyncReadDispatcher* dispatcher)
                : m_fileHandle(hFile)
                , m_origin(origin)
                , m_isReader(reader)
                , m_isWriter(writer)
                , m_dispatcher(dispatcher)
            {
            }

            POSIXFileHandle::~POSIXFileHandle()
            {
                close(m_fileHandle);
                m_fileHandle = 0;
            }

            int POSIXFileHandle::syncFileHandle() const
            {
                return m_fileHandle;
            }

            const StringBuf& POSIXFileHandle::originInfo() const
            {
                return m_origin;
            }

            uint64_t POSIXFileHandle::size() const
            {
                auto pos  = lseek64(m_fileHandle, 0, SEEK_CUR);
                auto size  = lseek64(m_fileHandle, 0, SEEK_END);
                lseek64(m_fileHandle, pos, SEEK_SET);
                return size;
            }

            uint64_t POSIXFileHandle::pos() const
            {
                return lseek64(m_fileHandle, 0, SEEK_CUR);
            }

            bool POSIXFileHandle::pos(uint64_t newPosition)
            {
                auto newPos  = lseek64(m_fileHandle, newPosition, SEEK_SET);
                return newPos == newPosition;
            }

            bool POSIXFileHandle::isReadingAllowed() const
            {
                return m_isReader;
            }

            bool POSIXFileHandle::isWritingAllowed() const
            {
                return m_isWriter;
            }

            uint64_t POSIXFileHandle::writeSync(const void* data, uint64_t size)
            {
                auto numWritten  = write(m_fileHandle, data, size);
                if (numWritten != size)
                {
                    TRACE_ERROR("Write failed for '{}', written {} instead of {} at {}, size {}: error: {}",
                            m_origin.c_str(), numWritten, size, pos(), size(), errno);
                }

                return numWritten;
            }

            uint64_t POSIXFileHandle::readSync(void* data, uint64_t size)
            {
                auto numRead  = read(m_fileHandle, data, size);
                if (numRead != size)
                {
                    TRACE_ERROR("Read failed for '{}'", m_origin);
                    return 0;
                }

                return numRead;
            }

            CAN_YIELD uint64_t POSIXFileHandle::readAsync(uint64_t offset, uint64_t size, void* readBuffer)
            {
                return m_dispatcher->readAsync(m_fileHandle, offset, size, readBuffer);
            }

        } // prv
    } // io
} // base

