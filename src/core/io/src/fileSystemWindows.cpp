/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#include "build.h"

#include "timestamp.h"
#include "fileFormat.h"

#include "core/containers/include/stringBuilder.h"
#include "core/containers/include/utf8StringFunctions.h"

#include "io.h"
#include "fileSystemWindows.h"
#include "fileIteratorWindows.h"
#include "fileHandleWindows.h"
#include "directoryWatcherWindows.h"
#include "asyncDispatcherWindows.h"

#include <shlobj.h>
#include <commdlg.h>

#pragma comment (lib, "Comdlg32.lib")

BEGIN_BOOMER_NAMESPACE()

namespace prv
{

    static bool GTraceIO = true;

    //--

    class TempPathStringBuffer
    {
    public:
        TempPathStringBuffer()
        {
            m_writePos = m_buffer;
            m_writeEnd = m_buffer + MAX_SIZE - 1;
            *m_writePos = 0;
        }

        TempPathStringBuffer(StringView view)
        {
            m_writePos = m_buffer;
            m_writeEnd = m_buffer + MAX_SIZE - 1;
            *m_writePos = 0;

            append(view);
        }

        void clear()
        {
            m_writePos = m_buffer;
            *m_writePos = 0;
        }

        operator const wchar_t* () const
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);
            return m_buffer;
        }

        wchar_t* pos() const
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);
            return m_writePos;
        }

        Array<wchar_t> buffer() const
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);
            auto count = (m_writePos - m_buffer) + 1;
            return Array<wchar_t>(m_buffer, count);
        }

        bool append(StringView txt)
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);

            auto start = m_writePos;

            auto ptr = txt.data();
            auto endPtr = txt.data() + txt.length();
            while (ptr < endPtr)
            {
                auto ch = utf8::NextChar(ptr, endPtr);
                if (m_writePos < m_writeEnd)
                {
                    if (ch == '/')
                        ch = '\\';

                    *m_writePos++ = (wchar_t)ch;
                    *m_writePos = 0;
                }
                else
                {
                    m_writePos = start;
                    m_writePos[1] = 0;
                    return false;
                }
            }

            return true;
        }

        bool append(const wchar_t* txt)
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);

            if (txt && *txt)
            {
                const auto len = wcslen(txt);
                if (m_writePos + len < m_writeEnd)
                {
                    while (*txt)
                    {
                        auto ch = *txt++;
                        if (ch == '/')
                            ch = '\\';

                        *m_writePos++ = ch;
                    }

                    *m_writePos = 0;
                }
                else
                {
                    return false;
                }
            }

            return true;
        }

        void pop(wchar_t* oldWritePos)
        {
            DEBUG_CHECK(oldWritePos >= m_buffer && oldWritePos <= m_writeEnd);
            m_writePos = oldWritePos;
            *m_writePos = 0;
        }

        void print(IFormatStream& f) const
        {
            f.append((const wchar_t*)m_buffer);
        }

    private:
        static const auto MAX_SIZE = MAX_PATH * 4;

        wchar_t m_buffer[MAX_SIZE];
        wchar_t* m_writePos;
        wchar_t* m_writeEnd;
    };

    //--

    class TempPathStringBufferAnsi
    {
    public:
        TempPathStringBufferAnsi()
        {
            m_writePos = m_buffer;
            m_writeEnd = m_buffer + MAX_SIZE - 1;
            *m_writePos = 0;
        }

        TempPathStringBufferAnsi(StringView view)
        {
            m_writePos = m_buffer;
            m_writeEnd = m_buffer + MAX_SIZE - 1;
            *m_writePos = 0;

            append(view);
        }

        void clear()
        {
            m_writePos = m_buffer;
            *m_writePos = 0;
        }

        operator const char* () const
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);
            return m_buffer;
        }

        char* pos() const
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);
            return m_writePos;
        }

        Array<char> buffer() const
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);
            auto count = (m_writePos - m_buffer) + 1;
            return Array<char>(m_buffer, count);
        }

        bool append(StringView txt)
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);

            auto start = m_writePos;

            auto ptr = txt.data();
            auto endPtr = txt.data() + txt.length();
            while (ptr < endPtr)
            {
                auto ch = *ptr++;

                if (ch == '/')
                    ch = '\\';

                if (m_writePos < m_writeEnd)
                {
                    *m_writePos++ = ch;
                    *m_writePos = 0;
                }
                else
                {
                    m_writePos = start;
                    m_writePos[1] = 0;
                    return false;
                }
            }

            return true;
        }

        bool append(const wchar_t* txt)
        {
            DEBUG_CHECK(m_writePos <= m_writeEnd);
            DEBUG_CHECK(*m_writePos == 0);

            if (txt && *txt)
            {
                auto* org = m_writePos;
                while (*txt)
                {
                    auto ch = *txt++;
                    if (ch == '/')
                        ch = '\\';

                    char data[8];
                    auto size = utf8::ConvertChar(data, ch);

                    if (m_writePos + size < m_writeEnd)
                    {
                        memcpy(m_writePos, data, size);
                        m_writePos += size;
                    }
                    else
                    {
                        m_writePos = org;
                        *m_writePos = 0;
                        return false;
                    }
                }

                *m_writePos = 0;
            }

            return true;
        }

        void pop(char* oldWritePos)
        {
            DEBUG_CHECK(oldWritePos >= m_buffer && oldWritePos <= m_writeEnd);
            m_writePos = oldWritePos;
            *m_writePos = 0;
        }

        void print(IFormatStream& f) const
        {
            f.append((const char*)m_buffer);
        }

    private:
        static const auto MAX_SIZE = MAX_PATH * 4;

        char m_buffer[MAX_SIZE];
        char* m_writePos;
        char* m_writeEnd;
    };

    //--

    WinIOSystem::WinIOSystem()
    {
        // create the dispatcher for async IO operations
        m_asyncDispatcher = new WinAsyncReadDispatcher(1024);
    }

    void WinIOSystem::deinit()
    {
        delete m_asyncDispatcher;
        m_asyncDispatcher = nullptr;
    }

    ReadFileHandlePtr WinIOSystem::openForReading(StringView absoluteFilePath, TimeStamp* outTimestamp)
    {
        TempPathStringBuffer str(absoluteFilePath);

        // Open file
        HANDLE handle = CreateFileW(str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
            TRACE_WARNING("WinIO: Failed to create reading handle for '{}', error 0x{}", absoluteFilePath, Hex(GetLastError()));
            return nullptr;
        }

        if (outTimestamp)
        {
            FILETIME fileTime;
            ::GetFileTime(handle, NULL, NULL, &fileTime);
            *outTimestamp = TimeStamp(*(const uint64_t*)&fileTime);
        }

        // Return file reader
        if (GTraceIO) TRACE_INFO("WinIO: Opened '{}' for reading", str);
        return RefNew<WinReadFileHandle>(handle, absoluteFilePath);
    }

    static void GenerateTempFilePath(StringView absoluteFilePath, TempPathStringBuffer& outStr)
    {
        // dir path
        outStr.append(absoluteFilePath.beforeLast("\\"));

        // file name
        static std::atomic<uint32_t> GLocalAppUniqueFile = 1;
        wchar_t tempFileName[MAX_PATH + 1];
        if (GetTempFileNameW(outStr, L"__BoomerTemp", GLocalAppUniqueFile++, tempFileName))
        {
            TRACE_INFO("WinIO: Generated temp file name: '{}'", (const wchar_t*)tempFileName);
            outStr.clear();
            outStr.append(tempFileName);
        }
        else
        {
            outStr.clear();
            outStr.append(absoluteFilePath);
            outStr.append(L".tmp");
        }
    }

    WriteFileHandlePtr WinIOSystem::openForWriting(const wchar_t* str, bool append)
    {
        // setup flags
        uint32_t winFlags = 0; // no sharing while writing
        uint32_t createFlags = append ? OPEN_ALWAYS : CREATE_ALWAYS;

        // Open file
        HANDLE handle = CreateFileW(str, GENERIC_WRITE, winFlags, NULL, createFlags, FILE_ATTRIBUTE_NORMAL, NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
            TRACE_WARNING("WinIO: Failed to create writing handle for '{}', error: 0x{}", str, Hex(GetLastError()));
            return nullptr;
        }

        // Move file pointer to the end in case of append
        if (append)
            SetFilePointer(handle, 0, 0, FILE_END);

        // Create the wrapper
        if (GTraceIO) TRACE_INFO("WinIO: Opened '{}' for writing", str);
        return RefNew<WinWriteFileHandle>(handle, "");
    }

    WriteFileHandlePtr WinIOSystem::openForWriting(StringView absoluteFilePath, FileWriteMode mode /*= FileWriteMode::StagedWrite*/)
    {
        TempPathStringBuffer str(absoluteFilePath);

        // Create path
        if (!createPath(absoluteFilePath))
        {
            TRACE_WARNING("WinIO: Failed to create path for '{}'", absoluteFilePath);
            return nullptr;
        }

        // Remove the read only flag
        if (!readOnlyFlag(absoluteFilePath, false))
        {
            TRACE_WARNING("WinIO: Unable to remove read only flag from file '{}', assuming it's protected", absoluteFilePath);
            return nullptr;
        }

        // Staged write
        if (mode == FileWriteMode::StagedWrite)
        {
            // generate temp file path and open it
            TempPathStringBuffer tempFilePath;
            GenerateTempFilePath(absoluteFilePath, tempFilePath);

            // create temp file writer
            auto tempFileWriter = openForWriting(tempFilePath, false);
            if (!tempFileWriter)
            {
                TRACE_WARNING("WinIO: Unable to create temp writing file for '{}', error: 0x{}", absoluteFilePath, Hex(GetLastError()));
                return nullptr;
            }

            // create wrapper
            if (GTraceIO) TRACE_INFO("WinIO: Opened '{}' for staged writing", str);
            return RefNew<WinWriteTempFileHandle>(str.buffer(), tempFilePath.buffer(), tempFileWriter);
        }

        // open file
        return openForWriting(str, mode == FileWriteMode::DirectAppend);
    }

    AsyncFileHandlePtr WinIOSystem::openForAsyncReading(StringView absoluteFilePath, TimeStamp* outTimestamp)
    {
        TempPathStringBuffer str(absoluteFilePath);

        // Open file
        HANDLE handle = CreateFileW(str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
            TRACE_WARNING("WinIO: Failed to create async reading handle for '{}', error: 0x{}", absoluteFilePath, Hex(GetLastError()));
            return nullptr;
        }

        LARGE_INTEGER size;
        if (!::GetFileSizeEx(handle, &size))
        {
            TRACE_WARNING("WinIO: Failed to get file size for '{}', error: 0x{}", str, Hex(GetLastError()));
            CloseHandle(handle);
            return 0;
        }

        if (outTimestamp)
        {
            FILETIME fileTime;
            ::GetFileTime(handle, NULL, NULL, &fileTime);
            *outTimestamp = TimeStamp(*(const uint64_t*)&fileTime);
        }

        // Return file reader
        if (GTraceIO) TRACE_INFO("WinIO: Opened '{}' for async reading ({})", str, MemSize(size.QuadPart));
        return RefNew<WinAsyncFileHandle>(handle, absoluteFilePath, size.QuadPart, m_asyncDispatcher);
    }

    //--

    Buffer WinIOSystem::openMemoryMappedForReading(StringView absoluteFilePath)
    {
        // TODO: right now just read into memory buffer
        return loadIntoMemoryForReading(absoluteFilePath);
    }

    Buffer WinIOSystem::loadIntoMemoryForReading(StringView absoluteFilePath)
    {
        TempPathStringBuffer str(absoluteFilePath);

        HANDLE hHandle = CreateFileW(str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hHandle == INVALID_HANDLE_VALUE)
        {
            TRACE_WARNING("WinIO: Failed to create reading handle for '{}', error: 0x{}", absoluteFilePath, Hex(GetLastError()));
            return nullptr;
        }

        uint64_t size;
        if (!GetFileSizeEx(hHandle, (PLARGE_INTEGER) &size))
        {
            TRACE_WARNING("WinIO: Unable to get size of file '{}', error: 0x{}", absoluteFilePath, Hex(GetLastError()));
            CloseHandle(hHandle);
            return nullptr;
        }

        auto ret = Buffer::Create(POOL_IO, size, 4096);
        if (!ret)
        {
            TRACE_WARNING("WinIO: Unable to allocate {} needed to load file '{}'", MemSize(size), absoluteFilePath);
            CloseHandle(hHandle);
            return nullptr;
        }

        uint32_t batchSize = 1024 * 1024 * 64;
        uint64_t bytesLeft = size;
        uint8_t* writeOffset = ret.data();
        while (bytesLeft > 0)
        {
            auto maxRead = (uint32_t) std::min<uint64_t>(bytesLeft, batchSize);
            bytesLeft -= maxRead;

            DWORD numRead = 0;
            if (!ReadFile(hHandle, writeOffset, maxRead, &numRead, NULL))
            {
                TRACE_WARNING("WinIO: IO error reading content of file '{}', error: 0x{}", absoluteFilePath, Hex(GetLastError()));
                CloseHandle(hHandle);
                return nullptr;
            }

            writeOffset += numRead;
        }

        if (GTraceIO) TRACE_INFO("WinIO: Loaded '{}' into memory ({})", absoluteFilePath, MemSize(size));
        CloseHandle(hHandle);
        return ret;
    }

    bool WinIOSystem::fileSize(StringView absoluteFilePath, uint64_t& outFileSize)
    {
        TempPathStringBuffer str(absoluteFilePath);

        // Open file
        HANDLE hHandle = CreateFileW(str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hHandle == INVALID_HANDLE_VALUE)
            return false;

        // Get file size  
        LARGE_INTEGER size;
        if (!GetFileSizeEx(hHandle, &size))
        {
            CloseHandle(hHandle);
            return false;
        }

        if (GTraceIO) TRACE_INFO("WinIO: FileSize '{}': {}", absoluteFilePath, size.QuadPart);

        // Return size
        outFileSize = size.QuadPart;
        CloseHandle(hHandle);
        return true;
    }

    bool WinIOSystem::fileTimeStamp(StringView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize)
    {
        TempPathStringBuffer str(absoluteFilePath);

        // Open file
        HANDLE hHandle = CreateFileW(str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        // Get file timestamp
        if (hHandle != INVALID_HANDLE_VALUE)
        {
            FILETIME fileTime;
            ::GetFileTime(hHandle, NULL, NULL, &fileTime);

            if (outFileSize)
                ::GetFileSizeEx(hHandle, (PLARGE_INTEGER)outFileSize);

            ::CloseHandle(hHandle);
            outTimeStamp = TimeStamp(*(const uint64_t*)&fileTime);

            if (GTraceIO) TRACE_INFO("WinIO: FileTimeStamp '{}': {}", absoluteFilePath, outTimeStamp);
            return true;
        }

        // Empty timestamp
        return false;
    }

    bool WinIOSystem::touchFile(StringView absoluteFilePath)
    {
        return true; // TODO
    }

    bool WinIOSystem::createPath(StringView absoluteFilePath)
    {
        TempPathStringBuffer str(absoluteFilePath);

        // Create path
        const wchar_t* path = str;
        for (const wchar_t *pos = path; *pos; pos++)
        {
            if (*pos == '\\' || *pos == '/')
            {
                auto old = *pos;
                *(wchar_t*)pos = 0;
                bool ok = CreateDirectoryW(path, NULL);
                *(wchar_t*)pos = old;

                if (!ok && !wcsrchr(path, ':'))
                    return false;
            }
        }

        // Path created
        //if (GTraceIO) TRACE_INFO("WinIO: FileTimeStamp '{}': {}", cstr, outTimeStamp);
        return true;
    }

    bool WinIOSystem::copyFile(StringView srcAbsolutePath, StringView destAbsolutePath)
    {
        ScopeTimer timer;

        // Delete destination file
        if (fileExists(destAbsolutePath) && !deleteFile(destAbsolutePath))
        {
            TRACE_WARNING("WinIO: FileCopy unable to delete destination file \"{}\"", destAbsolutePath);
            return false;
        }

        // Make sure target path exists
        if (!createPath(destAbsolutePath))
        {
            TRACE_WARNING("WinIO: FileCopy unable to create target path for file \"{}\"", destAbsolutePath);
            return false;
        }

        // Copy File
        TempPathStringBuffer srcStr(srcAbsolutePath);
        TempPathStringBuffer destStr(destAbsolutePath);
        if (0 == CopyFile(srcStr, destStr, FALSE))
        {
            TRACE_WARNING("WinIO: Unable to copy file \"{}\" to \"{}\": 0x{}", srcAbsolutePath, destAbsolutePath, Hex(GetLastError()));
            return false;
        }

        // file copied
        if (GTraceIO) TRACE_INFO("WinIO: FileCopy '{}' to '{}', {}", srcStr, destStr, timer);
        return true;
    }

    bool WinIOSystem::moveFile(StringView srcAbsolutePath, StringView destAbsolutePath)
    {
        ScopeTimer timer;

        // Delete destination file
        if (fileExists(destAbsolutePath) && !deleteFile(destAbsolutePath))
        {
            TRACE_WARNING("FileMove unable to delete destination file \"{}\"", destAbsolutePath);
            return false;
        }

        // Make sure target path exists
        if (!createPath(destAbsolutePath))
        {
            TRACE_WARNING("WinIO: FileMove unable to create target path for file \"{}\"", destAbsolutePath);
            return false;
        }

        // Move the file
        TempPathStringBuffer srcStr(srcAbsolutePath);
        TempPathStringBuffer destStr(destAbsolutePath);
        if (0 == ::MoveFileW(srcStr, destStr))
        {
            TRACE_WARNING("WinIO: Unable to move file \"{}\" to \"{}\": 0x{}", srcAbsolutePath, destAbsolutePath, Hex(GetLastError()));
            return false;
        }

        // File moved
        if (GTraceIO) TRACE_INFO("WinIO: FileCopy '{}' to '{}', {}", srcStr, destStr, timer);
        return true;
    }

    bool WinIOSystem::deleteFile(StringView absoluteFilePath)
    {
        if (!readOnlyFlag(absoluteFilePath, false))
            return false;

        TempPathStringBuffer cstr(absoluteFilePath);
        if (!::DeleteFileW(cstr))
        {
            TRACE_WARNING("WinIO: Unable to delete file '{}', error: 0x{}", cstr, Hex(GetLastError()));
            return false;
        }

        if (GTraceIO) TRACE_INFO("WinIO: FileDelete '{}'", cstr);
        return true;
    }

	bool WinIOSystem::deleteDir(StringView absoluteDirPath)
	{
        TempPathStringBuffer cstr(absoluteDirPath);
        if (!::RemoveDirectoryW(cstr))
        {
            TRACE_WARNING("WinIO: Unable to delete directory '{}', error: 0x{}", cstr, Hex(GetLastError()));
            return false;
        }

        if (GTraceIO) TRACE_INFO("WinIO: DirectoryDelete '{}'", cstr);
        return true;
	}

    bool WinIOSystem::fileExists(StringView absoluteFilePath)
    {
        TempPathStringBuffer cstr(absoluteFilePath);
        DWORD dwAttrib = GetFileAttributes(cstr);

        const auto exists = (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
        if (GTraceIO) TRACE_INFO("WinIO: FileExists '{}': {}", cstr, exists);

        return exists;
    }

    bool WinIOSystem::isFileReadOnly(StringView absoluteFilePath)
    {
        TempPathStringBuffer cstr(absoluteFilePath);

        auto attr = ::GetFileAttributesW(cstr);
        if (attr == INVALID_FILE_ATTRIBUTES)
            return false;

        bool readOnly = (attr & FILE_ATTRIBUTE_READONLY) != 0;
        if (GTraceIO) TRACE_INFO("WinIO: FileReadOnly '{}': {}", cstr, readOnly);

        return readOnly;
    }

    bool WinIOSystem::readOnlyFlag(StringView absoluteFilePath, bool flag)
    {
        TempPathStringBuffer cstr(absoluteFilePath);

        auto attr  = ::GetFileAttributesW(cstr);
        auto srcAttr  = attr;

        // Change read only flag
        if (flag)
            attr |= FILE_ATTRIBUTE_READONLY;
        else
            attr &= ~FILE_ATTRIBUTE_READONLY;

        // same ?
        if (attr == srcAttr)
            return true;

        if (SetFileAttributes(cstr, attr) != 0)
        {
            TRACE_WARNING("WinIO: Unable to set read-only attribute of file \"{}\": 0x{}", absoluteFilePath, Hex(GetLastError()));
            return false;
        }

        if (GTraceIO) TRACE_INFO("WinIO: SetFileReadOnly '{}': {}", cstr, flag);
        return true;
    }

    bool WinIOSystem::findFilesInternal(TempPathStringBuffer& dirPath, TempPathStringBufferAnsi& dirPathUTF, StringView searchPattern, const std::function<bool(StringView fullPath, StringView fileName)>& enumFunc, bool recurse)
    {
        auto* org = dirPath.pos();
        auto* org2 = dirPathUTF.pos();
        if (!dirPath.append(searchPattern))
            return false;
        if (!dirPathUTF.append(searchPattern))
        {
            dirPath.pop(org);
            return false;
        }

        for (WinFileIterator it(dirPath, true, false); it; ++it)
        {
            dirPath.pop(org);
            dirPathUTF.pop(org2);

            const auto* fileName = it.fileName();

            if (dirPath.append(fileName) && dirPathUTF.append(fileName))
                if (enumFunc((const char*)dirPathUTF, fileName))
                    return true;
        }

        if (recurse && dirPath.append(L"*."))
        {
            for (WinFileIterator it(dirPath, false, true); it; ++it)
            {
                dirPath.pop(org);
                dirPathUTF.pop(org2);

                if (dirPath.append("\\") && dirPath.append(it.fileNameRaw()))
                {
                    if (dirPathUTF.append("\\") && dirPathUTF.append(it.fileNameRaw()))
                    {
                        if (findFilesInternal(dirPath, dirPathUTF, searchPattern, enumFunc, recurse))
                            return true;
                    }
                }
            }
        }

        return false;
    }

    bool WinIOSystem::findFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView fullPath, StringView fileName)>& enumFunc, bool recurse)
    {
        if (absoluteFilePath.empty())
            return false;

        TempPathStringBuffer dirPath(absoluteFilePath);
        TempPathStringBufferAnsi dirPath2(absoluteFilePath);
        return findFilesInternal(dirPath, dirPath2, searchPattern, enumFunc, recurse);
    }

    bool WinIOSystem::findSubDirs(StringView absoluteFilePath, const std::function<bool(StringView name)>& enumFunc)
    {
        TempPathStringBuffer dirPath(absoluteFilePath);
        if (dirPath.append(L"*."))
        {
            for (WinFileIterator it(dirPath, false, true); it; ++it)
                if (enumFunc(it.fileName()))
                    return true;
        }

        return false;
    }

    bool WinIOSystem::findLocalFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView name)>& enumFunc)
    {
        TempPathStringBuffer dirPath(absoluteFilePath);
        if (dirPath.append(searchPattern))
        {
            for (WinFileIterator it(dirPath, true, false); it; ++it)
                if (enumFunc(it.fileName()))
                    return true;
        }

        return false;
    }

    void WinIOSystem::systemPath(PathCategory category, IFormatStream& f)
    {
        wchar_t path[MAX_PATH+1];

        switch (category)
        {
            case PathCategory::ExecutableFile:
            {
                GetModuleFileNameW(NULL, path, MAX_PATH);
                f.append(path);
                break;
            }

            case PathCategory::ExecutableDir:
            {
                GetModuleFileNameW(NULL, path, MAX_PATH);

                if (auto* ch = wcsrchr(path, '\\'))
                    ch[1] = 0;

                f.append(path);
                break;
            }

            case PathCategory::SharedDir:
            {
                GetModuleFileNameW(NULL, path, MAX_PATH);

                if (auto* ch = wcsrchr(path, '\\'))
                    ch[0] = 0;

                if (auto* ch = wcsrchr(path, '\\'))
                    ch[1] = 0;

                f.append(path);
                f.append("shared\\");
                break;
            }

            case PathCategory::EngineDir:
            {
                GetModuleFileNameW(NULL, path, MAX_PATH);

                for (;;)
                {
                    auto* ch = wcsrchr(path, '\\');
                    if (!ch)
                        break;

                    ch[1] = 0;
                    wcscat(ch, L"project.xml");

                    DWORD dwAttrib = GetFileAttributes(path);
                    const auto exists = (dwAttrib != INVALID_FILE_ATTRIBUTES) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
                    if (!exists)
                    {
                        ch[0] = 0;
                    }
                    else
                    {
                        ch[1] = 0;
                        f.append(path);
                        break;
                    }                            
                }

                break;
            }

            case PathCategory::SystemTempDir:
			{
				wchar_t path[MAX_PATH + 1];
				auto length = GetTempPathW(MAX_PATH, path);

				if (length > 0)
				{
					wcscat(path, L"Boomer\\");
					f << path;
					break;
				}

				// !!!!!
				// FALL THROUGH TO LOCAL TEMP DIR
			}
                    
			case PathCategory::LocalTempDir:
			{
				wchar_t path[MAX_PATH + 1];
				GetModuleFileNameW(NULL, path, MAX_PATH);

				if (auto* ch = wcsrchr(path, '\\'))
					ch[1] = 0;

				wcscat(path, L".temp\\local\\");
				f << path;
				break;
			}

            case PathCategory::UserConfigDir:
            {
                wchar_t path[MAX_PATH + 1];

                HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
                ASSERT(result == S_OK);
                wcscat_s(path, MAX_PATH, L"\\Boomer\\config\\");

                f << path;
                break;
            }

            case PathCategory::UserDocumentsDir:
            {
                wchar_t path[MAX_PATH + 1];

                HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
                ASSERT(result == S_OK);
                wcscat_s(path, MAX_PATH, L"\\");

                f << path;
                break;
            }
        }
    }

    DirectoryWatcherPtr WinIOSystem::createDirectoryWatcher(StringView path)
    {
        TempPathStringBuffer str(path);
        return RefNew<prv::WinDirectoryWatcher>(str.buffer());
    }

    void WinIOSystem::showFileExplorer(StringView path)
    {
        TempPathStringBuffer cstr(path);

        auto pidl  = ILCreateFromPathW(cstr);
        if (pidl)
        {
            SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
            ILFree(pidl);
        }
    }

    namespace helper
    {
        class PreserveCurrentDirectory
        {
        public:
            PreserveCurrentDirectory(StringView dirToSet)
            {
                TempPathStringBuffer cstr(dirToSet);
                GetCurrentDirectoryW(MAX_STRING, m_currentDirectory);
                SetCurrentDirectoryW(cstr);
            }

            ~PreserveCurrentDirectory()
            {
                SetCurrentDirectoryW(m_currentDirectory);
            }

        private:
            static const uint32_t MAX_STRING = 512;
            wchar_t m_currentDirectory[MAX_STRING];
        };

        static void AppendUni(Array<wchar_t>& outFormatString, const wchar_t* buf)
        {
            auto length  = wcslen(buf);
            auto destStr  = outFormatString.allocateUninitialized(range_cast<uint32_t>(length));
            memcpy(destStr, buf, sizeof(wchar_t) * length);
        }

		static void AppendUni(Array<wchar_t>& outFormatString, const char* buf)
		{
			auto length  = strlen(buf);
			auto destStr  = outFormatString.allocateUninitialized(range_cast<uint32_t>(length));
			for (size_t i = 0; i < length; ++i)
				*destStr++ = buf[i];
		}

        static void AppendFormatStrings(Array<wchar_t>& outFormatString, Array<StringBuf> outFormatNames, int& outCurrentFilterIndex, const Array<FileFormat>& formats, const StringBuf& currentFilter, bool allowMultipleFormats)
        {
            int currentFilterIndex = 1;

            if (!formats.empty())
            {
                if (allowMultipleFormats && (formats.size() > 1))
                {
                    StringBuilder formatDisplayString;
                    StringBuilder formatFilterString;

                    if (currentFilter == "AllSupported" || currentFilter == "")
                        outCurrentFilterIndex = currentFilterIndex;

                    for (auto &format : formats)
                    {
                        if (!formatFilterString.empty())
                            formatFilterString.append(";");
                        formatFilterString.appendf("*.{}", format.extension());

                        if (!formatDisplayString.empty())
                            formatDisplayString.append(", ");
                        formatDisplayString.appendf("*.{}", format.extension());
                    }

                    // all supported files
                    AppendUni(outFormatString, L"All supported files [");
                    AppendUni(outFormatString, formatDisplayString.c_str());
                    AppendUni(outFormatString, L"]");
                    outFormatString.pushBack(0);

                    AppendUni(outFormatString, UTF16StringVector(formatFilterString.c_str()).c_str());
                    outFormatString.pushBack(0);

                    outFormatNames.pushBack(StringBuf("AllSupported"));
                    currentFilterIndex += 1;
                }

                for (auto &format : formats)
                {
                    if (format.extension() == currentFilter)
                        outCurrentFilterIndex = currentFilterIndex;

                    AppendUni(outFormatString, format.description().c_str());
                    AppendUni(outFormatString, L" [*.");
                    AppendUni(outFormatString, format.extension().c_str());
                    AppendUni(outFormatString, L"]");
                    outFormatString.pushBack(0);

                    AppendUni(outFormatString, L"*.");
                    AppendUni(outFormatString, format.extension().c_str());
                    outFormatString.pushBack(0);

                    outFormatNames.pushBack(format.extension());
                    currentFilterIndex += 1;
                }
            }

            if (/*allowMultipleFormats || */formats.empty())
            {
                if (currentFilter == "All")
                    outCurrentFilterIndex = currentFilterIndex;

                AppendUni(outFormatString, L"All files");
                outFormatString.pushBack(0);
                AppendUni(outFormatString, L"*.*");

                outFormatNames.pushBack(StringBuf("All"));
                currentFilterIndex += 1;
            }

            // end of format list
            outFormatString.pushBack(0);
            outFormatString.pushBack(0);
        }

        static void ExtractFilePaths(const WCHAR* resultBuffer, Array<StringBuf>& outPaths)
        {
            Array<const WCHAR*> parts;

            // Parse file names
            auto pos  = resultBuffer;
            auto start  = resultBuffer;
            while (1)
            {
                if (0 == *pos)
                {
                    if (pos == start)
                        break;

                    parts.pushBack(start);
                    pos += 1;
                    start = pos;
                }
                else
                {
                    ++pos;
                }
            }

            if (parts.size() == 1)
            {
                // single path case
                outPaths.emplaceBack(parts[0]);
            }
            else if (parts.size() > 1)
            {
                // in multiple paths case the first entry is the directory path and the rest are the file names
                for (uint32_t i = 1; i < parts.size(); ++i)
                {
                    StringBuilder txt;
                    txt << parts[0];
                    txt << "\\";
                    txt << parts[i];
                    outPaths.emplaceBack(txt.toString());
                }
            }
        }

    } // helper

    bool WinIOSystem::showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, Array<StringBuf>& outPaths, OpenSavePersistentData& persistentData)
    {
        // build filter string
        Array<wchar_t> formatString;
        Array<StringBuf> formatNames;
        int formatFilterIndex = -1;
        helper::AppendFormatStrings(formatString, formatNames, formatFilterIndex, formats, persistentData.filterExtension, true);

        // buffer for user pattern
        wchar_t userFilterPattern[128];
        wcscpy_s(userFilterPattern, ARRAY_COUNT(userFilterPattern), (const wchar_t*)persistentData.userPattern.c_str());

        // buffer for selected files
        Array<wchar_t> fileNamesBuffer;
        fileNamesBuffer.resizeWith(65536, 0);

        // preserve directory (hack)
        helper::PreserveCurrentDirectory dirHelper(persistentData.directory);
        TempPathStringBuffer initialDirPath(persistentData.directory.c_str());

        OPENFILENAMEW info;
        memset(&info, 0, sizeof(info));
        info.lStructSize = sizeof(info);
        info.hwndOwner = (HWND)nativeWindowHandle;
        info.lpstrFilter = formatString.typedData();
        info.lpstrCustomFilter = userFilterPattern;
        info.nMaxCustFilter = ARRAY_COUNT(userFilterPattern);
        info.nFilterIndex = formatFilterIndex;
        info.lpstrFile = fileNamesBuffer.typedData();
        info.nMaxFile = fileNamesBuffer.size() - 1;
        info.lpstrInitialDir = initialDirPath;
        info.Flags = (allowMultiple ? OFN_ALLOWMULTISELECT : 0) | OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON;
        if (!GetOpenFileNameW(&info))
        {
            TRACE_WARNING("GetOpenFileName returned false");
            return false;
        }
                
        // extract file paths
        helper::ExtractFilePaths(info.lpstrFile, outPaths);

        // update selected filter
        if (info.nFilterIndex >= 1 && info.nFilterIndex <= formatNames.size())
            persistentData.filterExtension = formatNames[info.nFilterIndex - 1];
        else
            persistentData.filterExtension = StringBuf();

        // update selected path
        //persistentData.directory = outPaths[0];
                
        // update custom filter
        persistentData.userPattern = StringBuf(info.lpstrCustomFilter);
        return true;
    }

    bool WinIOSystem::showFileSaveDialog(uint64_t nativeWindowHandle, const StringBuf& currentFileName, const Array<FileFormat>& formats, StringBuf& outPath, OpenSavePersistentData& persistentData)
    {
        // use the file extension as default filter in fallback conditions
        auto filterName = persistentData.filterExtension;
        if (filterName.empty())
            filterName = StringBuf(currentFileName.stringAfterFirst(".").c_str());

        // build filter string
        Array<wchar_t> formatString;
        Array<StringBuf> formatNames;
        int formatFilterIndex = -1;
        helper::AppendFormatStrings(formatString, formatNames, formatFilterIndex, formats, persistentData.filterExtension, false);

        // buffer for user pattern
        wchar_t userFilterPattern[128];
        {
            TempPathStringBuffer temp(persistentData.userPattern.c_str());
            wcscpy_s(userFilterPattern, ARRAY_COUNT(userFilterPattern), (const wchar_t*)temp);
        }

        // buffer for selected files
        Array<wchar_t> fileNamesBuffer;
        fileNamesBuffer.resizeWith(65536, 0);
                
        // setup current file name
        {
            TempPathStringBuffer temp(persistentData.userPattern.c_str());
            wcscpy_s(fileNamesBuffer.typedData(), fileNamesBuffer.size(), (const wchar_t*)temp);
        }

        TempPathStringBuffer initialDirectoryPath(persistentData.directory.c_str());

        // preserve directory (hack)
        helper::PreserveCurrentDirectory dirHelper(persistentData.directory);

        OPENFILENAMEW info;
        memset(&info, 0, sizeof(info));
        info.lStructSize = sizeof(info);
        info.hwndOwner = (HWND)nativeWindowHandle;
        info.lpstrFilter = formatString.typedData();
        info.lpstrCustomFilter = userFilterPattern;
        info.nMaxCustFilter = ARRAY_COUNT(userFilterPattern);
        info.nFilterIndex = formatFilterIndex;
        info.lpstrFile = fileNamesBuffer.typedData();
        info.nMaxFile = fileNamesBuffer.size() - 1;
        info.lpstrInitialDir = initialDirectoryPath;
        info.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_NONETWORKBUTTON;
        if (!GetSaveFileNameW(&info))
        {
            TRACE_WARNING("GetOpenFileName returned false");
            return false;
        }

        // extract file paths
        auto path = StringBuf(info.lpstrFile);
        if (path.empty())
        {
            TRACE_WARNING("GetOpenFileName returned invalid path '{}'", info.lpstrFile);
            return false;
        }

        // update selected filter
        if (info.nFilterIndex >= 1 && info.nFilterIndex <= formatNames.size())
            persistentData.filterExtension = formatNames[info.nFilterIndex - 1];
        else
            persistentData.filterExtension = StringBuf();

        // update selected path
        persistentData.directory = path.stringBeforeLast("\\");
        outPath = path;

        // update custom filter
        persistentData.userPattern = StringBuf(info.lpstrCustomFilter);
        return true;
    }

} // prv

END_BOOMER_NAMESPACE()
