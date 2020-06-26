/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#include "build.h"
#include "ioFileHandleWindows.h"
#include "ioAsyncDispatcherWindows.h"
#include "base/system/include/timing.h"

namespace base
{
    namespace io
    {
        namespace prv
        {


            WinFileHandle::WinFileHandle(HANDLE hSyncFile, HANDLE hAsyncFile, const StringBuf& origin, bool reader, bool writer, bool locked, WinAsyncReadDispatcher* dispatcher)
                : m_hSyncFile(hSyncFile)
                , m_hAsyncFile(hAsyncFile)
                , m_origin(origin)
                , m_isReader(reader)
                , m_isWriter(writer)
                , m_isLocked(locked)
                , m_dispatcher(dispatcher)
            {
                InitializeCriticalSection(&m_asyncHandleLock);
            }

            WinFileHandle::~WinFileHandle()
            {
                if (m_isLocked)
                {
                    if (!UnlockFile(m_hSyncFile, 0, 0, ~0UL, ~0UL))
                    {
                        TRACE_WARNING("IO: Failed to unlock '{}'", m_origin);
                    }
                }

                if (m_hSyncFile != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(m_hSyncFile);
                    m_hSyncFile = INVALID_HANDLE_VALUE;
                }

                if (m_hAsyncFile != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(m_hAsyncFile);
                    m_hAsyncFile = INVALID_HANDLE_VALUE;
                }

                DeleteCriticalSection(&m_asyncHandleLock);
            }

            HANDLE WinFileHandle::syncFileHandle() const
            {
                return m_hSyncFile;
            }

            HANDLE WinFileHandle::asyncFileHandle() const
            {
                return m_hAsyncFile;
            }

            const StringBuf& WinFileHandle::originInfo() const
            {
                return m_origin;
            }

            uint64_t WinFileHandle::size() const
            {
                LARGE_INTEGER size;
                if (!::GetFileSizeEx(m_hSyncFile, &size))
                {
                    TRACE_ERROR("Failed to get file size for '{}'", m_origin);
                    return 0;
                }

                return (uint64_t)size.QuadPart;
            }

            uint64_t WinFileHandle::pos() const
            {
                LARGE_INTEGER pos;
                pos.QuadPart = 0;

                if (!::SetFilePointerEx(m_hSyncFile, pos, &pos, FILE_CURRENT))
                {
                    TRACE_ERROR("Failed to get file position for '{}'", m_origin);
                    return 0;
                }

                return (uint64_t)pos.QuadPart;
            }

            bool WinFileHandle::pos(uint64_t newPosition)
            {
                LARGE_INTEGER pos;
                pos.QuadPart = newPosition;

                if (!::SetFilePointerEx(m_hSyncFile, pos, &pos, FILE_BEGIN))
                {
                    TRACE_ERROR("Failed to seek file position for '{}'", m_origin);
                    return false;
                }

                return true;
            }

            bool WinFileHandle::isReadingAllowed() const
            {
                return m_isReader;
            }

            bool WinFileHandle::isWritingAllowed() const
            {
                return m_isWriter;
            }

            uint64_t WinFileHandle::writeSync(const void* data, uint64_t size)
            {
                // the size of the IO operation is limited
                if (size > MAX_IO_SIZE)
                {
                    TRACE_ERROR("Trying to write block larger than the maximum allowed on this platform");
                    return 0;
                }

                // write data
                DWORD bytesWritten = 0;
                if (!WriteFile(m_hSyncFile, data, range_cast<uint32_t>(size), &bytesWritten, NULL))
                {
                    TRACE_ERROR("Write failed for '{}'", m_origin);
                    return 0;
                }

                return bytesWritten;
            }

            uint64_t WinFileHandle::readSync(void* data, uint64_t size)
            {
                // the size of the IO operation is limited
                if (size > MAX_IO_SIZE)
                {
                    TRACE_ERROR("Trying to read block larger than the maximum allowed on this platform");
                    return 0;
                }

                // read data
                DWORD byteaRead = 0;
                if (!ReadFile(m_hSyncFile, data, range_cast<uint32_t>(size), &byteaRead, NULL))
                {
                    TRACE_ERROR("Read failed for '{}'", m_origin);
                    return 0;
                }

                return byteaRead;
            }

            uint64_t WinFileHandle::readAsync(uint64_t offset, uint64_t size, void* readBuffer)
            {
				if (m_hAsyncFile == INVALID_HANDLE_VALUE)
                    return 0;

                return m_dispatcher->readAsync(m_hSyncFile, m_hAsyncFile, offset, size, readBuffer);
            }

        } // prv
    } // io
} // base

