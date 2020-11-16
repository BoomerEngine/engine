/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: posix #]
***/

#include "build.h"

#include "StringBuf.h"
#include "StringBufBuilder.h"
#include "timestamp.h"
#include "fileFormat.h"
#include "utils.h"

#include "ioSystemPOSIX.h"
#include "ioFileIteratorPOSIX.h"
#include "ioFileHandlePOSIX.h"
#include "ioAsyncDispatcherPOSIX.h"
#include "ioDirectoryWatcherPOSIX.h"

#include "base/containers/include/utf8StringFunctions.h"
#include "base/containers/include/stringBuilder.h"

#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/time.h>
#include <utime.h>
#include <pwd.h>
#include <ftw.h>

namespace base
{
    namespace io
    {
        namespace prv
        {

            //---

            POSIXIOSystem::POSIXIOSystem()
            {
                // initialize async dispatcher
                m_asyncDispatcher = CreateUniquePtr<POSIXAsyncReadDispatcher>(4096);
            }

            POSIXIOSystem::~POSIXIOSystem()
            {
                m_asyncDispatcher.reset();
            }

            bool POSIXIOSystem::servicesPath(const StringBuf& absoluteFilePath)
            {
                // TODO: path should start with proper drive name
                return true;
            }

            FileHandlePtr POSIXIOSystem::openForReading(const StringBuf& absoluteFilePath)
            {
                // convert file path
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), absoluteFilePath.c_str(), absoluteFilePath.view().length());

                // open file
                auto handle  = open(filePath, O_RDONLY);
                if (handle <= 0)
                {
                    TRACE_ERROR("Failed to create reading handle for '{}'", absoluteFilePath);
                    return nullptr;
                }

                // Return file reader
                return CreateSharedPtr<POSIXFileHandle>(handle, absoluteFilePath.ansi_str().c_str(), true, false, m_asyncDispatcher.get());
            }

            FileHandlePtr POSIXIOSystem::openForWriting(const StringBuf& absoluteFilePath, bool append)
            {
                // Create path
                if (!createPath(absoluteFilePath))
                {
                    TRACE_ERROR("Failed to create path for '{}'", absoluteFilePath);
                    return nullptr;
                }

                // Remove the read only flag
                readOnlyFlag(absoluteFilePath, false);

                // convert file path
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), absoluteFilePath.c_str(), absoluteFilePath.view().length());

                // Open file
                auto handle  = open(filePath, O_WRONLY | O_CREAT | (append ? 0 : O_TRUNC), DEFFILEMODE);
                if (handle <= 0)
                {
                    auto ret  = errno;
                    TRACE_ERROR("Failed to create writing handle for '{}': {}", filePath, ret);
                    return nullptr;
                }

                // Move file pointer
                uint32_t pos = 0;
                if (append)
                    pos = lseek64(handle, 0, SEEK_END);

                // Create the wrapper
                return CreateSharedPtr<POSIXFileHandle>(handle, absoluteFilePath.ansi_str().c_str(), false, true, m_asyncDispatcher.get());
            }

            FileHandlePtr POSIXIOSystem::openForReadingAndWriting(const StringBuf& absoluteFilePath, bool resetContent /*= false*/)
            {
                // Create path
                if (!createPath(absoluteFilePath))
                {
                    TRACE_ERROR("Failed to create path for '{}'", absoluteFilePath);
                    return nullptr;
                }

                // Remove the read only flag
                readOnlyFlag(absoluteFilePath, false);

                // convert file path
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), absoluteFilePath.c_str(), absoluteFilePath.view().length());

                // Open file
                int handle = open(filePath, O_RDWR | O_CREAT, DEFFILEMODE);
                if (handle <= 0)
                {
                    TRACE_ERROR("Failed to create read/write handle for '{}'", absoluteFilePath);
                    return nullptr;
                }

                // Create the wrapper
                return CreateSharedPtr<POSIXFileHandle>(handle, absoluteFilePath.ansi_str().c_str(), true, true, m_asyncDispatcher.get());
            }

            bool POSIXIOSystem::fileSize(const StringBuf& absoluteFilePath, uint64_t& outFileSize)
            {
                // convert file path
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), absoluteFilePath.c_str(), absoluteFilePath.view().length());

                // get file stats
                struct stat st;
                if (0 != stat(filePath, &st))
                    return false;

                outFileSize = st.st_size;
                return true;
            }

            bool POSIXIOSystem::fileTimeStamp(const StringBuf& absoluteFilePath, class TimeStamp& outTimeStamp)
            {
                // convert file path
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), absoluteFilePath.c_str(), absoluteFilePath.view().length());

                // get file stats
                struct stat st;
                if (0 != stat(filePath, &st))
                    return false;

                outTimeStamp = TimeStamp::GetFromFileTime(st.st_mtim.tv_sec, st.st_mtim.tv_nsec);
                return true;
            }

            bool POSIXIOSystem::createPath(const StringBuf& absoluteFilePath)
            {
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), absoluteFilePath.c_str(), absoluteFilePath.view().length());

                // Create path
                char* path = filePath;
                for (char *pos = path; *pos; pos++)
                {
                    if (*pos == '\\' || *pos == '/')
                    {
                        char was = *pos;
                        *pos = 0;

                        if (strlen(path) > 0)
                        {
                            if (0 != mkdir(path, ALLPERMS))
                            {
                                auto ret  = errno;
                                if (ret != EEXIST)
                                {
                                    TRACE_ERROR("Failed to create path '{}': {}", path, ret);
                                    return false;
                                }
                            }
                        }

                        *pos = was;
                    }
                }

                // Path created
                return true;
            }

            bool POSIXIOSystem::moveFile(const StringBuf& srcStringBuf, const StringBuf& destStringBuf)
            {
                // convert source path
                char srcFilePath[512];
                utf8::FromUniChar(srcFilePath, sizeof(srcFilePath), srcStringBuf.c_str(), srcStringBuf.view().length());

                // convert destination path
                char destFilePath[512];
                utf8::FromUniChar(destFilePath, sizeof(destFilePath), destStringBuf.c_str(), destStringBuf.view().length());

                // Delete destination file
                if (-1 != access(destFilePath, F_OK))
                {
                    if (0 != remove(destFilePath))
                    {
                        auto err  = errno;
                        TRACE_ERROR("Unable to move file \"{}\" to \"{}\". Destination file cannot be removed. Error: {}", srcStringBuf, destStringBuf, err);
                        return false;
                    }
                }

                // create target path
                if (!createPath(destStringBuf))
                {
                    TRACE_ERROR("Unable to move file \"{}\" to \"{}\", Unable to create target path", srcStringBuf, destStringBuf);
                    return false;
                }

                // Move the file
                if (0 != rename(srcFilePath, destFilePath))
                {
                    auto err  = errno;
                    TRACE_ERROR("Unable to move file \"{}\" to \"{}\", Error: {}", srcStringBuf, destStringBuf, err);
                    return false;
                }

                // File moved
                return true;
            }

            bool POSIXIOSystem::deleteFile(const StringBuf& absoluteFilePath)
            {
                if (!readOnlyFlag(absoluteFilePath, false))
                    return false;

                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), absoluteFilePath.c_str(), absoluteFilePath.view().length());

                // Delete the file
                if (0 != remove(filePath))
                {
                    auto err  = errno;
                    TRACE_ERROR("Unable to delete file \"{}\", Error: {}", absoluteFilePath, err);
                    return false;
                }

                // File deleted
                return true;
            }

            static int RemoveRecursive(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
            {
                int rv = remove(fpath);

                if (rv)
                {
                    auto err = errno;
                    TRACE_ERROR("Unable to delete \"{}\", Error: {}", fpath, err);
                }

                return rv;
            }

            bool POSIXIOSystem::deleteDir(const StringBuf& abosluteDirPath)
            {
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), abosluteDirPath.c_str(), abosluteDirPath.view().length());

                // delete all stuff
                if (0 != nftw(filePath, RemoveRecursive, 64, FTW_DEPTH | FTW_PHYS))
                    return false;

                // all content deleted
                return true;
            }

            bool POSIXIOSystem::touchFile(const StringBuf& absoluteFilePath)
            {
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), absoluteFilePath.c_str(), absoluteFilePath.view().length());

                time_t curTime = 0;
                time(&curTime);

                utimbuf buf;
                buf.modtime = curTime;
                buf.actime = curTime;
                utime(filePath, &buf);

                return true;
            }

            bool POSIXIOSystem::fileExists(const StringBuf& absoluteFilePath)
            {
                char filePath[512];
                utf8::FromUniChar(filePath, sizeof(filePath), absoluteFilePath.c_str(), absoluteFilePath.view().length());
                return (-1 != access(filePath, F_OK));
            }

            bool POSIXIOSystem::isFileReadOnly(const StringBuf& absoluteFilePath)
            {
                /*auto attr = ::GetFileAttributesW(absoluteFilePath.c_str());
                if (attr == INVALID_FILE_ATTRIBUTES)
                    return false;

                // Check attribute
                return (attr & FILE_ATTRIBUTE_READONLY) != 0;*/
                return false;
            }

            bool POSIXIOSystem::readOnlyFlag(const StringBuf& absoluteFilePath, bool flag)
            {
                /*auto attr = ::GetFileAttributesW(absoluteFilePath.c_str());
                auto srcAttr = attr;

                // Change read only flag
                if (flag)
                    attr |= FILE_ATTRIBUTE_READONLY;
                else
                    attr &= ~FILE_ATTRIBUTE_READONLY;

                // same ?
                if (attr == srcAttr)
                    return true;

                if (SetFileAttributes(absoluteFilePath.c_str(), attr) != 0)
                {
                    TRACE_WARNING("unable to set read-only attribute of file \"{}\"", absoluteFilePath);
                    return false;
                }*/

                return true;
            }

            void POSIXIOSystem::findFiles(const StringBuf& absoluteFilePath, const wchar_t* searchPattern, Array< StringBuf >& absoluteFiles, bool recurse)
            {
                for (POSIXFileIterator it(absoluteFilePath, searchPattern, true, false); it; ++it)
                    absoluteFiles.pushBack(it.filePath());

                // Recurse to directories
                if (recurse)
                {
                    for (POSIXFileIterator it(absoluteFilePath, L"*.", false, true); it; ++it)
                    {
                        auto newAbsoluteFilePath = absoluteFilePath.addDir(UTF16StringBuf(it.fileName()));
                        findFiles(newAbsoluteFilePath, searchPattern, absoluteFiles, recurse);
                    }
                }
            }

            void POSIXIOSystem::findSubDirs(const StringBuf& absoluteFilePath, Array< UTF16StringBuf >& outDirectoryNames)
            {
                for (POSIXFileIterator it(absoluteFilePath, L"*.", false, true); it; ++it)
                    outDirectoryNames.emplaceBack(it.fileName());
            }

            void POSIXIOSystem::findLocalFiles(const StringBuf& absoluteFilePath, const wchar_t* searchPattern, Array< UTF16StringBuf >& outFileNames)
            {
                for (POSIXFileIterator it(absoluteFilePath, searchPattern, true, false); it; ++it)
                    outFileNames.emplaceBack(it.fileName());
            }

            static UTF16StringBuf GetHomeDirectory()
            {
                const char *homedir;
                if ((homedir = getenv("HOME")) == NULL)
                    homedir = getpwuid(getuid())->pw_dir;

                UTF16StringBuf ret(homedir);
                ret += "/";
                return ret;
            }

            StringBuf POSIXIOSystem::systemPath(PathCategory category)
            {
                char buffer[512];

                switch (category)
                {
                    case PathCategory::ExecutableFile:
                    {
                        auto length = readlink("/proc/self/exe", buffer, 512);
                        buffer[length] = 0;
                        return io::StringBuf::Build(UTF16StringBuf(buffer));
                    }

                    case PathCategory::ExecutableDir:
                    {
                        return systemPath(PathCategory::ExecutableFile).basePath();
                    }

                    case PathCategory::TempDir:
                    {
                        auto path = StringBuf::Build("/var/tmp/BoomerEngine/");
                        createPath(path);
                        return path;
                    }

                    case PathCategory::UserConfigDir:
                    {
                        auto path = StringBuf::Build(GetHomeDirectory());
                        return path.addDir(L"BoomerEngine").addDir(L"config");
                    }
                }

                return StringBuf();
            }

            DirectoryWatcherPtr POSIXIOSystem::createDirectoryWatcher(const StringBuf& path)
            {
                return base::CreateSharedPtr<POSIXDirectoryWatcher>(path);
            }

            void POSIXIOSystem::showFileExplorer(const StringBuf& path)
            {
                // TODO: add support for soemthing more than Ubuntu :(

                auto command = BaseTempString<4096>("nautilus -w {}", path.c_str());
                TRACE_INFO("Executing command '{}'", command.c_str());
                FILE *f = popen(command.c_str(), "r");
                fclose(f);
            }

            static void AppenFormatStrings(StringBuilder& builder, const Array<FileFormat>& formats, bool allowMultipleFormats)
            {
                if (!formats.empty())
                {
                    if (allowMultipleFormats && (formats.size() > 1))
                    {
                        base::StringBuilder formatDisplayString;
                        base::StringBuilder formatFilterString;

                        for (auto &format : formats)
                        {
                            formatFilterString.appendf(" | *.{}", format.extension());

                            if (!formatDisplayString.empty())
                                formatDisplayString.append(",");
                            formatDisplayString.appendf("*.{}", format.extension());
                        }

                        builder.appendf("--file-filter=\"All supported formats | {}\" ", formatFilterString.c_str());
                    }

                    for (auto &format : formats)
                        builder.appendf("--file-filter=\"{} [*.{}] | *.{}\" ", format.description(), format.extension(), format.extension());

                    if (allowMultipleFormats)
                        builder.append("--file-filter=\"All files [*.*] | *.*\" ");
                }
            }

            static void SliceString(const wchar_t* str, wchar_t breakCh, Array< UTF16StringBuf >& outTokens)
            {
                const wchar_t* start = str;
                for (;; )
                {
                    // get the char from the text
                    wchar_t ch = *str;

                    // it's a break char
                    if (ch == breakCh)
                    {
                        // emit token
                        auto length = str - start;
                        outTokens.emplaceBack(start, (uint32_t)length);

                        // next token can start after this char
                        start = str + 1;
                    }

                    // that was it
                    if (!*str )
                    {
                        // emit final token
                        auto length = str - start;
                        outTokens.emplaceBack(start, (uint32_t)length);
                        break;
                    }

                    // move to next char
                    str += 1;
                }
            }

            bool POSIXIOSystem::showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<StringBuf>& outPaths, OpenSavePersistentData& persistentData)
            {
                //zenity --file-selection --filename=dupa --multiple --file-filter="Portable Network Graphics [*.png] | *.png" --file-filter="Zip [*.zip] | *.zip"

                // change directory
                if (!persistentData.m_directory.empty())
                {
                    StringBuf ansiPath(persistentData.m_directory.c_str());
                    if (0 != chdir(ansiPath.c_str()))
                    {
                        TRACE_WARNING("Failed to enter directory {}'", persistentData.m_directory);
                    }
                }

                // format command
                base::StringBuilder builder;
                builder.appendf("zenity --file-selection --modal --attach={} ", nativeWindowHandle);
                if (allowMultiple)
                    builder.append("--multiple ");

                AppenFormatStrings(builder, formats, true);

                // get command
                auto command = builder.toString();
                TRACE_INFO("Executing command '{}'", command);

                // show the dialog
                FILE *f = popen(command.c_str(), "r");
                if (NULL == f)
                    return false;

                // parse result
                base::Array<char> buffer;
                buffer.reserve(65536);
                while (!feof(f))
                {
                    // load data from stream
                    char data[4096];
                    if (!fgets(data, sizeof(data), f))
                        break;

                    auto read = data;
                    while (*read && *read != '\n')
                        buffer.pushBack(*read++);

                    if (*read == '\n')
                        break;
                }
                buffer.pushBack(0);

                // convert to unicode
                auto uniString = UTF16StringBuf(buffer.typedData());
                base::Array<UTF16StringBuf> paths;
                SliceString(uniString.c_str(), '|', paths);

                // emit paths
                for (auto& path : paths)
                {
                    auto finalPath = StringBuf::Build(path);
                    if (!finalPath.empty())
                        outPaths.pushBack(finalPath);
                }

                // update the directory
                if (!outPaths.empty())
                {
                    persistentData.m_directory = outPaths[0].basePath();
                    persistentData.m_filterExtension = StringBuf(outPaths[0].fileExtensions().c_str());
                }

                // true if we have at least one valid path
                return !outPaths.empty();
            }

            bool POSIXIOSystem::showFileSaveDialog(uint64_t nativeWindowHandle, const UTF16StringBuf& currentFileName, const Array<FileFormat>& formats, StringBuf& outPath, OpenSavePersistentData& persistentData)
            {
                // change directory
                if (!persistentData.m_directory.empty())
                {
                    StringBuf ansiPath(persistentData.m_directory.c_str());
                    if (0 != chdir(ansiPath.c_str()))
                    {
                        TRACE_WARNING("Failed to enter directory {}'", persistentData.m_directory);
                    }
                }

                // format command
                base::StringBuilder builder;
                builder.appendf("zenity --file-selection --modal --save --attach={} ", nativeWindowHandle);
                if (!currentFileName.empty())
                    builder.appendf("--filename=\"{}\" ", currentFileName);
                AppenFormatStrings(builder, formats, false);

                // get command
                auto command = builder.toString();
                TRACE_INFO("Executing command '{}'", command);

                // show the dialog
                FILE *f = popen(command.c_str(), "r");
                if (NULL == f)
                    return false;

                // load data from stream
                char data[4096];
                if (NULL == fgets(data, sizeof(data), f))
                    return false;

                // remove the '\n'
                char* ptr = data;
                while (*ptr != 0)
                {
                    if (*ptr == '\n')
                    {
                        *ptr = 0;
                        break;
                    }

                    ++ptr;
                }

                // convert to path
                auto path = StringBuf::Build(UTF16StringBuf(data));
                if (path.empty())
                    return false;

                // update the directory
                persistentData.m_directory = path.basePath();
                persistentData.m_filterExtension = StringBuf(path.fileExtensions().c_str());

                // valid save path selected
                outPath = path;
                return true;
            }

        } // prv
    } // io
} // base