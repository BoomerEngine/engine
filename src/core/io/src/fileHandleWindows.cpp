/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#include "build.h"
#include "fileHandleWindows.h"
#include "asyncDispatcherWindows.h"
#include "core/system/include/timing.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{

    //--

    WinReadFileHandle::WinReadFileHandle(HANDLE hSyncFile, const StringView path)
        : m_hHandle(hSyncFile)
        , m_origin(path)
    {
    }

    WinReadFileHandle::~WinReadFileHandle()
    {
        if (m_hHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hHandle);
            m_hHandle = INVALID_HANDLE_VALUE;
        }
    }

    uint64_t WinReadFileHandle::size() const
    {
        LARGE_INTEGER size;
        if (!::GetFileSizeEx(m_hHandle, &size))
        {
            TRACE_WARNING("WinIO: Failed to get file size for '{}', error: 0x{}", m_origin, Hex(GetLastError()));
            return 0;
        }

        return (uint64_t)size.QuadPart;
    }

    uint64_t WinReadFileHandle::pos() const
    {
        LARGE_INTEGER pos;
        pos.QuadPart = 0;

        if (!::SetFilePointerEx(m_hHandle, pos, &pos, FILE_CURRENT))
        {
            TRACE_WARNING("WinIO: Failed to get file position for '{}', error: 0x{}", m_origin, Hex(GetLastError()));
            return 0;
        }

        return (uint64_t)pos.QuadPart;
    }

    bool WinReadFileHandle::pos(uint64_t newPosition)
    {
        LARGE_INTEGER pos;
        pos.QuadPart = newPosition;

        if (!::SetFilePointerEx(m_hHandle, pos, &pos, FILE_BEGIN))
        {
            TRACE_WARNING("WinIO: Failed to seek file position for '{}', error: 0x{}", m_origin, Hex(GetLastError()));
            return false;
        }

        return true;
    }

    uint64_t WinReadFileHandle::readSync(void* data, uint64_t size)
    {
        const uint64_t ONE_READ_MAX = 1ULL << 30; // 1GB

        uint64_t totalDataRead = 0;
        while (size > 0)
        {
            // read data
            DWORD bytesRead = 0;
            const auto readSize = (uint32_t) std::min<uint64_t>(ONE_READ_MAX, size);
            if (!ReadFile(m_hHandle, data, readSize, &bytesRead, NULL))
            {
                TRACE_WARNING("WinIO: Read failed for '{}' at offset {}, read size {}, error: 0x{}", m_origin, pos(), readSize, Hex(GetLastError()));
                break;
            }

            // accumulate total read count
            totalDataRead += bytesRead;
            size -= bytesRead;

            // not enough data read ?
            if (bytesRead != readSize)
            {
                TRACE_WARNING("WinIO: Read was incomplete for '{}' at offset {}, read size {} but got {}", m_origin, pos(), readSize, bytesRead);
                break;
            }
        }

        return totalDataRead;
    }

    //--

    WinWriteFileHandle::WinWriteFileHandle(HANDLE hSyncFile, StringView path)
        : m_hHandle(hSyncFile)
        , m_origin(path)
    {
    }

    WinWriteFileHandle::~WinWriteFileHandle()
    {
        if (m_hHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hHandle);
            m_hHandle = INVALID_HANDLE_VALUE;
        }
    }

    uint64_t WinWriteFileHandle::size() const
    {
        LARGE_INTEGER size;
        if (!::GetFileSizeEx(m_hHandle, &size))
        {
            TRACE_WARNING("WinIO: Failed to get file size for '{}', error: 0x{}", m_origin, Hex(GetLastError()));
            return 0;
        }

        return (uint64_t)size.QuadPart;
    }

    uint64_t WinWriteFileHandle::pos() const
    {
        LARGE_INTEGER pos;
        pos.QuadPart = 0;

        if (!::SetFilePointerEx(m_hHandle, pos, &pos, FILE_CURRENT))
        {
            TRACE_WARNING("WinIO: Failed to get file position for '{}', error: 0x{}", m_origin, Hex(GetLastError()));
            return 0;
        }

        return (uint64_t)pos.QuadPart;
    }

    bool WinWriteFileHandle::pos(uint64_t newPosition)
    {
        LARGE_INTEGER pos;
        pos.QuadPart = newPosition;

        if (!::SetFilePointerEx(m_hHandle, pos, &pos, FILE_BEGIN))
        {
            TRACE_WARNING("WinIO: Failed to seek file position for '{}', error: 0x{}", m_origin, Hex(GetLastError()));
            return false;
        }

        return true;
    }

    uint64_t WinWriteFileHandle::writeSync(const void* data, uint64_t size)
    {
        const uint64_t ONE_WRITE_MAX = 1ULL << 30; // 1GB

        uint64_t totalDataWritten = 0;
        while (size)
        {
            // write data
            DWORD bytesWritten = 0;
            const auto writeSize = (uint32_t)std::min<uint64_t>(ONE_WRITE_MAX, size);
            if (!WriteFile(m_hHandle, data, writeSize, &bytesWritten, NULL))
            {
                TRACE_WARNING("WinIO: Write failed for '{}' at offset {}, write size {}, error: 0x{}", m_origin, pos(), writeSize, Hex(GetLastError()));
                break;
            }

            // accumulate total read count
            totalDataWritten += bytesWritten;
            size -= bytesWritten;

            // not enough data read ?
            if (bytesWritten != writeSize)
            {
                TRACE_WARNING("WinIO: Write was incomplete for '{}' at offset {}, read size {} but got {}", m_origin, pos(), writeSize, bytesWritten);
                break;
            }
        }

        return totalDataWritten;
    }

    void WinWriteFileHandle::discardContent()
    {
        TRACE_WARNING("WinIO: Requested to discard content of non-dicardable write '{}'", m_origin);
    }

    //--

    WinWriteTempFileHandle::WinWriteTempFileHandle(Array<wchar_t> targetPath, Array<wchar_t> tempFilePath, const WriteFileHandlePtr& tempFileWriter)
        : m_tempFileWriter(tempFileWriter)
        , m_tempFilePath(tempFilePath)
        , m_targetFilePath(targetPath)
    {}

    WinWriteTempFileHandle::~WinWriteTempFileHandle()
    {
        if (m_tempFileWriter)
        {
            // close it
            m_tempFileWriter.reset();

            // delete target file
            DeleteFile(m_targetFilePath.typedData());

            // move temp file to the target place
            if (MoveFile(m_tempFilePath.typedData(), m_targetFilePath.typedData()))
            {
                TRACE_INFO("WinIO: Finished staged writing for target '{}'. Temp file '{}' will be delete.", 
                    m_targetFilePath.typedData(), m_tempFilePath.typedData());
            }
            else
            {
                TRACE_WARNING("WinIO: Failed to move starged file to '{}'. New content remains saved at '{}'.",
                    m_targetFilePath.typedData(), m_tempFilePath.typedData());
            }
        }
    }

    uint64_t WinWriteTempFileHandle::size() const
    {
        if (m_tempFileWriter)
            return m_tempFileWriter->size();
        return 0;
    }

    uint64_t WinWriteTempFileHandle::pos() const
    {
        if (m_tempFileWriter)
            return m_tempFileWriter->pos();
        return 0;
    }

    bool WinWriteTempFileHandle::pos(uint64_t newPosition)
    {
        if (m_tempFileWriter)
            return m_tempFileWriter->pos(newPosition);
        return false;
    }

    uint64_t WinWriteTempFileHandle::writeSync(const void* data, uint64_t size)
    {
        if (m_tempFileWriter)
            return m_tempFileWriter->writeSync(data, size);
        return 0;
    }

    void WinWriteTempFileHandle::discardContent()
    {
        if (m_tempFileWriter)
        {
            TRACE_WARNING("WinIO: Discarded file writing for target '{}'. Temp file '{}' will be delete.");
            m_tempFileWriter.reset();
            DeleteFile(m_tempFilePath.typedData());
        }
    }

    //--

    WinAsyncFileHandle::WinAsyncFileHandle(HANDLE hAsyncFile, StringView origin, uint64_t size, WinAsyncReadDispatcher* dispatcher)
        : m_hHandle(hAsyncFile)
        , m_origin(origin)
        , m_size(size)
        , m_dispatcher(dispatcher)
    {
    }

    WinAsyncFileHandle::~WinAsyncFileHandle()
    {
        if (m_hHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hHandle);
            m_hHandle = INVALID_HANDLE_VALUE;
        }
    }

    uint64_t WinAsyncFileHandle::size() const
    {
        return m_size;
    }

    uint64_t WinAsyncFileHandle::readAsync(uint64_t offset, uint64_t size, void* readBuffer)
    {
        return m_dispatcher->readAsync(m_hHandle, offset, size, readBuffer);
    }

    //--

} // prv

END_BOOMER_NAMESPACE()
