/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#include "build.h"

#include "absolutePath.h"
#include "absolutePathBuilder.h"
#include "timestamp.h"
#include "fileFormat.h"

#include "base/containers/include/stringBuilder.h"

#include "ioSystem.h"
#include "ioSystemWindows.h"
#include "ioFileIteratorWindows.h"
#include "ioFileHandleWindows.h"
#include "ioDirectoryWatcherWindows.h"
#include "ioAsyncDispatcherWindows.h"

#include <shlobj.h>
#include <commdlg.h>

#pragma comment (lib, "Comdlg32.lib")

namespace base
{
    namespace io
    {
        namespace prv
        {

            static bool GTraceIO = true;

            //--

            class TempPathStringBuffer
            {
            public:
                TempPathStringBuffer(const AbsolutePathView& view, bool forceCopy = false)
                {
                    m_writePos = m_buffer;
                    m_writeEnd = m_buffer + MAX_PATH - 1;

                    if (!view.empty())
                    {
                        if (!forceCopy && view.data()[view.length()] == 0) // zero terminated
                        {
                            m_ret = view.data();
                        }
                        else
                        {
                            DEBUG_CHECK_EX(view.length() < MAX_PATH, "Absolute path length is longer than the system maximum");
                            const auto maxCopy = std::min<uint32_t>(MAX_PATH - 1, view.length());

                            memcpy(m_buffer, view.data(), maxCopy * sizeof(wchar_t));
                            m_buffer[maxCopy] = 0;
                            m_ret = m_buffer;
                            m_writePos = m_buffer + maxCopy;
                        }
                    }
                }

                operator const wchar_t* () const
                {
                    return m_ret;
                }

                StringView<wchar_t> view() const
                {
                    if (m_ret != m_buffer)
                        return StringView<wchar_t>(m_ret);
                    else
                        return StringView<wchar_t>(m_buffer, m_writePos);
                }

                wchar_t* pos() const
                {
                    return m_writePos;
                }

                bool append(StringView<wchar_t> txt)
                {
                    if (m_writePos + txt.length() >= m_writeEnd)
                        return false;

                    memcpy(m_writePos, txt.data(), txt.length() * sizeof(wchar_t));
                    m_writePos += txt.length();
                    *m_writePos = 0;
                    return true;
                }

                void pop(wchar_t* oldWritePos)
                {
                    DEBUG_CHECK(oldWritePos >= m_buffer && oldWritePos <= m_writeEnd);
                    m_writePos = oldWritePos;
                }

                void print(IFormatStream& f) const
                {
                    f.append(m_ret);
                }

            private:
                wchar_t m_buffer[MAX_PATH];
                wchar_t* m_writePos;
                wchar_t* m_writeEnd;

                const wchar_t* m_ret = L"";
            };

            //--

            WinIOSystem::WinIOSystem()
            {
                // create the dispatcher for async IO operations
                m_asyncDispatcher = MemNew(WinAsyncReadDispatcher, 1024);
            }

            void WinIOSystem::deinit()
            {
                MemDelete(m_asyncDispatcher);
                m_asyncDispatcher = nullptr;
            }

            ReadFileHandlePtr WinIOSystem::openForReading(AbsolutePathView absoluteFilePath)
            {
                UTF16StringBuf cstr(absoluteFilePath);

                // Open file
                HANDLE handle = CreateFileW(cstr.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (handle == INVALID_HANDLE_VALUE)
                {
                    TRACE_WARNING("WinIO: Failed to create reading handle for '{}', error 0x{}", absoluteFilePath, Hex(GetLastError()));
                    return nullptr;
                }

                // Return file reader
                if (GTraceIO) TRACE_INFO("WinIO: Opened '{}' for reading", cstr);
                return CreateSharedPtr<WinReadFileHandle>(handle, std::move(cstr));
            }

            static UTF16StringBuf GenerateTempFilePath(AbsolutePathView absoluteFilePath)
            {
                UTF16StringBuf cstr(absoluteFilePath.beforeLast(L"\\"));

                static std::atomic<uint32_t> GLocalAppUniqueFile = 1;
                WCHAR tempFileName[MAX_PATH + 1];
                if (GetTempFileName(cstr.c_str(), L"__BoomerTemp", GLocalAppUniqueFile++, tempFileName))
                {
                    TRACE_INFO("WinIO: Generated temp file name: '{}'", (const wchar_t*)tempFileName);
                    return UTF16StringBuf(tempFileName);
                }
                else
                {
                    UTF16StringBuf cstr(absoluteFilePath);
                    cstr += L".tmp";
                    return cstr;
                }
            }


            WriteFileHandlePtr WinIOSystem::openForWriting(AbsolutePathView absoluteFilePath, FileWriteMode mode /*= FileWriteMode::StagedWrite*/)
            {
                UTF16StringBuf cstr(absoluteFilePath);

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
                    auto tempFilePath = GenerateTempFilePath(absoluteFilePath);
                    auto tempFileWriter = openForWriting(tempFilePath, FileWriteMode::DirectWrite);
                    if (!tempFileWriter)
                    {
                        TRACE_WARNING("WinIO: Unable to create temp writing file for '{}', error: 0x{}", absoluteFilePath, Hex(GetLastError()));
                        return nullptr;
                    }

                    // create wrapper
                    if (GTraceIO) TRACE_INFO("WinIO: Opened '{}' for staged writing", cstr);
                    return CreateSharedPtr<WinWriteTempFileHandle>(std::move(cstr), std::move(tempFilePath), tempFileWriter);
                }

                // setup flags
                uint32_t winFlags = 0; // no sharing while writing
                uint32_t createFlags = (mode == FileWriteMode::DirectAppend) ? OPEN_ALWAYS : CREATE_ALWAYS;

                // Open file
                HANDLE handle = CreateFileW(cstr.c_str(), GENERIC_WRITE, winFlags, NULL, createFlags, FILE_ATTRIBUTE_NORMAL, NULL);
                if (handle == INVALID_HANDLE_VALUE)
                {
                    TRACE_WARNING("WinIO: Failed to create writing handle for '{}', error: 0x{}", absoluteFilePath, Hex(GetLastError()));
                    return nullptr;
                }

                // Move file pointer to the end in case of append
                if (mode == FileWriteMode::DirectAppend)
                    SetFilePointer(handle, 0, 0, FILE_END);

                // Create the wrapper
                if (GTraceIO) TRACE_INFO("WinIO: Opened '{}' for writing", cstr);
                return CreateSharedPtr<WinWriteFileHandle>(handle, std::move(cstr));
            }

            AsyncFileHandlePtr WinIOSystem::openForAsyncReading(AbsolutePathView absoluteFilePath)
            {
                UTF16StringBuf cstr(absoluteFilePath);

                // Open file
                HANDLE handle = CreateFileW(cstr.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
                if (handle == INVALID_HANDLE_VALUE)
                {
                    TRACE_WARNING("WinIO: Failed to create async reading handle for '{}', error: 0x{}", absoluteFilePath, Hex(GetLastError()));
                    return nullptr;
                }

                LARGE_INTEGER size;
                if (!::GetFileSizeEx(handle, &size))
                {
                    TRACE_WARNING("WinIO: Failed to get file size for '{}', error: 0x{}", cstr, Hex(GetLastError()));
                    CloseHandle(handle);
                    return 0;
                }

                // Return file reader
                if (GTraceIO) TRACE_INFO("WinIO: Opened '{}' for async reading ({})", cstr, MemSize(size.QuadPart));
                return CreateSharedPtr<WinAsyncFileHandle>(handle, std::move(cstr), size.QuadPart, m_asyncDispatcher);
            }

            //--

            Buffer WinIOSystem::openMemoryMappedForReading(AbsolutePathView absoluteFilePath)
            {
                // TODO: right now just read into memory buffer
                return loadIntoMemoryForReading(absoluteFilePath);
            }

            Buffer WinIOSystem::loadIntoMemoryForReading(AbsolutePathView absoluteFilePath)
            {
                TempPathStringBuffer cstr(absoluteFilePath);

                HANDLE hHandle = CreateFileW(cstr, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

                if (GTraceIO) TRACE_INFO("WinIO: Loaded '{}' into memory ({})", cstr, MemSize(size));
                CloseHandle(hHandle);
                return ret;
            }

            bool WinIOSystem::fileSize(AbsolutePathView absoluteFilePath, uint64_t& outFileSize)
            {
                TempPathStringBuffer cstr(absoluteFilePath);

                // Open file
                HANDLE hHandle = CreateFileW(cstr, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hHandle == INVALID_HANDLE_VALUE)
                    return false;

                // Get file size  
                LARGE_INTEGER size;
                if (!GetFileSizeEx(hHandle, &size))
                {
                    CloseHandle(hHandle);
                    return false;
                }

                if (GTraceIO) TRACE_INFO("WinIO: FileSize '{}': {}", cstr, size.QuadPart);

                // Return size
                outFileSize = size.QuadPart;
                CloseHandle(hHandle);
                return true;
            }

            bool WinIOSystem::fileTimeStamp(AbsolutePathView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize)
            {
                TempPathStringBuffer cstr(absoluteFilePath);

                // Open file
                HANDLE hHandle = CreateFileW(cstr, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                // Get file timestamp
                if (hHandle != INVALID_HANDLE_VALUE)
                {
                    FILETIME fileTime;
                    ::GetFileTime(hHandle, NULL, NULL, &fileTime);

                    if (outFileSize)
                        ::GetFileSizeEx(hHandle, (PLARGE_INTEGER)outFileSize);

                    ::CloseHandle(hHandle);
                    outTimeStamp = TimeStamp(*(const uint64_t*)&fileTime);

                    if (GTraceIO) TRACE_INFO("WinIO: FileTimeStamp '{}': {}", cstr, outTimeStamp);
                    return true;
                }

                // Empty timestamp
                return false;
            }

            bool WinIOSystem::touchFile(AbsolutePathView absoluteFilePath)
            {
                return true; // TODO
            }

            bool WinIOSystem::createPath(AbsolutePathView absoluteFilePath)
            {
                TempPathStringBuffer cstr(absoluteFilePath);

                // Create path
                const wchar_t* path = cstr;
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

            bool WinIOSystem::copyFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath)
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

            bool WinIOSystem::moveFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath)
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

            bool WinIOSystem::deleteFile(AbsolutePathView absoluteFilePath)
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

			bool WinIOSystem::deleteDir(AbsolutePathView absoluteDirPath)
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

            bool WinIOSystem::fileExists(AbsolutePathView absoluteFilePath)
            {
                TempPathStringBuffer cstr(absoluteFilePath);
                DWORD dwAttrib = GetFileAttributes(cstr);

                const auto exists = (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
                if (GTraceIO) TRACE_INFO("WinIO: FileExists '{}': {}", cstr, exists);

                return exists;
            }

            bool WinIOSystem::isFileReadOnly(AbsolutePathView absoluteFilePath)
            {
                TempPathStringBuffer cstr(absoluteFilePath);

                auto attr = ::GetFileAttributesW(cstr);
                if (attr == INVALID_FILE_ATTRIBUTES)
                    return false;

                bool readOnly = (attr & FILE_ATTRIBUTE_READONLY) != 0;
                if (GTraceIO) TRACE_INFO("WinIO: FileReadOnly '{}': {}", cstr, readOnly);

                return readOnly;
            }

            bool WinIOSystem::readOnlyFlag(AbsolutePathView absoluteFilePath, bool flag)
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

            bool WinIOSystem::findFilesInternal(TempPathStringBuffer& dirPath, StringView<wchar_t> searchPattern, const std::function<bool(AbsolutePathView fullPath, StringView<wchar_t> fileName)>& enumFunc, bool recurse)
            {
                auto* org = dirPath.pos();
                if (!dirPath.append(searchPattern))
                    return false;

                for (WinFileIterator it(dirPath, true, false); it; ++it)
                {
                    dirPath.pop(org);
                    if (dirPath.append(it.fileName()))
                        if (enumFunc(dirPath.view(), it.fileName()))
                            return true;
                }

                if (recurse && dirPath.append(L"*."))
                {
                    for (WinFileIterator it(dirPath, false, true); it; ++it)
                    {
                        dirPath.pop(org);
                        if (dirPath.append(L"\\") && dirPath.append(it.fileName()))
                        {
                            if (findFilesInternal(dirPath, searchPattern, enumFunc, recurse))
                                return true;
                        }
                    }
                }

                return false;
            }

            bool WinIOSystem::findFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(AbsolutePathView fullPath, StringView<wchar_t> fileName)>& enumFunc, bool recurse)
            {
                if (absoluteFilePath.empty())
                    return false;

                TempPathStringBuffer dirPath(absoluteFilePath, true);
                return findFilesInternal(dirPath, searchPattern, enumFunc, recurse);
            }

            bool WinIOSystem::findSubDirs(AbsolutePathView absoluteFilePath, const std::function<bool(StringView<wchar_t> name)>& enumFunc)
            {
                TempPathStringBuffer dirPath(absoluteFilePath, true);
                if (dirPath.append(L"*."))
                {
                    for (WinFileIterator it(dirPath, false, true); it; ++it)
                        if (enumFunc(it.fileName()))
                            return true;
                }

                return false;
            }

            bool WinIOSystem::findLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(StringView<wchar_t> name)>& enumFunc)
            {
                TempPathStringBuffer dirPath(absoluteFilePath, true);
                if (dirPath.append(searchPattern))
                {
                    for (WinFileIterator it(dirPath, true, false); it; ++it)
                        if (enumFunc(it.fileName()))
                            return true;
                }

                return false;
            }

            AbsolutePath WinIOSystem::systemPath(PathCategory category)
            {
                wchar_t path[MAX_PATH+1];

                switch (category)
                {
                    case PathCategory::ExecutableFile:
                    {
                        GetModuleFileNameW(NULL, path, MAX_PATH);
                        return AbsolutePath::Build(path);
                    }

                    case PathCategory::ExecutableDir:
                    {
                        GetModuleFileNameW(NULL, path, MAX_PATH);
                        return AbsolutePath::Build(path).basePath();
                    }

                    case PathCategory::TempDir:
                    {
                        wchar_t path[MAX_PATH + 1];
                        auto length = GetTempPathW(MAX_PATH, path);

                        if (length > 0)
                        {
                            AbsolutePathBuilder builder(path);
                            builder.pushDirectory(UTF16StringBuf("Boomer"));
                            return builder.toAbsolutePath();
                        }
                        else
                        {
                            GetModuleFileNameW(NULL, path, MAX_PATH);
                            return AbsolutePath::Build(path).basePath().addDir(".temp").addDir("local");
                        }
                    }

                    case PathCategory::UserConfigDir:
                    {
                        wchar_t path[MAX_PATH + 1];

                        HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
                        ASSERT(result == S_OK);
                        wcscat_s(path, MAX_PATH, L"\\");

                        AbsolutePathBuilder builder(path);
                        builder.pushDirectory(UTF16StringBuf("Boomer"));
                        builder.pushDirectory(UTF16StringBuf("config"));

                        return builder.toAbsolutePath();
                    }

                    case PathCategory::UserDocumentsDir:
                    {
                        wchar_t path[MAX_PATH + 1];

                        HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
                        ASSERT(result == S_OK);
                        wcscat_s(path, MAX_PATH, L"\\");

                        AbsolutePathBuilder builder(path);
                        return builder.toAbsolutePath();
                    }
                }

                return AbsolutePath();
            }

            DirectoryWatcherPtr WinIOSystem::createDirectoryWatcher(AbsolutePathView path)
            {                                      
                return base::CreateSharedPtr<prv::WinDirectoryWatcher>(path);
            }

            void WinIOSystem::showFileExplorer(AbsolutePathView path)
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
                    PreserveCurrentDirectory(AbsolutePathView dirToSet)
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

                static void AppendUni(base::Array<wchar_t>& outFormatString, const wchar_t* buf)
                {
                    auto length  = wcslen(buf);
                    auto destStr  = outFormatString.allocateUninitialized(range_cast<uint32_t>(length));
                    memcpy(destStr, buf, sizeof(wchar_t) * length);
                }

				static void AppendUni(base::Array<wchar_t>& outFormatString, const char* buf)
				{
					auto length  = strlen(buf);
					auto destStr  = outFormatString.allocateUninitialized(range_cast<uint32_t>(length));
					for (size_t i = 0; i < length; ++i)
						*destStr++ = buf[i];
				}

                static void AppendFormatStrings(base::Array<wchar_t>& outFormatString, base::Array<StringBuf> outFormatNames, int& outCurrentFilterIndex, const Array<FileFormat>& formats, const base::StringBuf& currentFilter, bool allowMultipleFormats)
                {
                    int currentFilterIndex = 1;

                    if (!formats.empty())
                    {
                        if (allowMultipleFormats && (formats.size() > 1))
                        {
                            base::StringBuilder formatDisplayString;
                            base::StringBuilder formatFilterString;

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

                            AppendUni(outFormatString, UTF16StringBuf(formatFilterString.c_str()).c_str());
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

                static void ExtractFilePaths(const WCHAR* resultBuffer, Array<AbsolutePath>& outPaths)
                {
                    base::Array<const WCHAR*> parts;

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
                        outPaths.pushBack(AbsolutePath::Build(parts[0]));
                    }
                    else if (parts.size() > 1)
                    {
                        // in multiple paths case the first entry is the directory path and the rest are the file names
                        auto basePath = AbsolutePath::BuildAsDir(parts[0]);
                        for (uint32_t i = 1; i < parts.size(); ++i)
                            outPaths.emplaceBack(basePath.addFile(parts[i]));
                    }
                }

            } // helper

            bool WinIOSystem::showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<AbsolutePath>& outPaths, OpenSavePersistentData& persistentData)
            {
                // build filter string
                base::Array<wchar_t> formatString;
                base::Array<StringBuf> formatNames;
                int formatFilterIndex = -1;
                helper::AppendFormatStrings(formatString, formatNames, formatFilterIndex, formats, persistentData.filterExtension, true);

                // buffer for user pattern
                wchar_t userFilterPattern[128];
                wcscpy_s(userFilterPattern, ARRAY_COUNT(userFilterPattern), (const wchar_t*)persistentData.userPattern.c_str());

                // buffer for selected files
                base::Array<wchar_t> fileNamesBuffer;
                fileNamesBuffer.resizeWith(65536, 0);

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
                info.lpstrInitialDir = persistentData.directory.c_str();
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
                    persistentData.filterExtension = base::StringBuf();

                // update selected path
                persistentData.directory = outPaths[0].basePath();
                
                // update custom filter
                persistentData.userPattern = info.lpstrCustomFilter;
                return true;
            }

            bool WinIOSystem::showFileSaveDialog(uint64_t nativeWindowHandle, const UTF16StringBuf& currentFileName, const Array<FileFormat>& formats, AbsolutePath& outPath, OpenSavePersistentData& persistentData)
            {
                // use the file extension as default filter in fallback conditions
                auto filterName = persistentData.filterExtension;
                if (filterName.empty())
                    filterName = StringBuf(currentFileName.stringAfterFirst(L".").c_str());

                // build filter string
                base::Array<wchar_t> formatString;
                base::Array<StringBuf> formatNames;
                int formatFilterIndex = -1;
                helper::AppendFormatStrings(formatString, formatNames, formatFilterIndex, formats, persistentData.filterExtension, false);

                // buffer for user pattern
                wchar_t userFilterPattern[128];
                wcscpy_s(userFilterPattern, ARRAY_COUNT(userFilterPattern), persistentData.userPattern.c_str());

                // buffer for selected files
                base::Array<wchar_t> fileNamesBuffer;
                fileNamesBuffer.resizeWith(65536, 0);
                
                // setup current file name
				wcscpy_s(fileNamesBuffer.typedData(), fileNamesBuffer.size(), currentFileName.c_str());

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
                info.lpstrInitialDir = persistentData.directory.c_str();
                info.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_NONETWORKBUTTON;
                if (!GetSaveFileNameW(&info))
                {
                    TRACE_WARNING("GetOpenFileName returned false");
                    return false;
                }

                // extract file paths
                auto path = AbsolutePath::Build(info.lpstrFile);
                if (path.empty())
                {
                    TRACE_WARNING("GetOpenFileName returned invalid path '{}'", info.lpstrFile);
                    return false;
                }

                // update selected filter
                if (info.nFilterIndex >= 1 && info.nFilterIndex <= formatNames.size())
                    persistentData.filterExtension = formatNames[info.nFilterIndex - 1];
                else
                    persistentData.filterExtension = base::StringBuf();

                // update selected path
                persistentData.directory = path.basePath();
                outPath = path;

                // update custom filter
                persistentData.userPattern = info.lpstrCustomFilter;
                return true;
            }

        } // prv
    } // io
} // base