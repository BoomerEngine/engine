/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot\filesystem #]
***/

#include "build.h"
#include "depotStructure.h"
#include "depotFileSystem.h"
#include "depotFileSystemNative.h"

#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/io/include/crcCache.h"
#include "base/io/include/absolutePath.h"
#include "base/containers/include/inplaceArray.h"

namespace base
{
    namespace depot
    {

        //--

        struct LocalAbsolutePathBuilder
        {
            static const uint32_t MAX_LENGTH = 256;

            wchar_t m_path[MAX_LENGTH];
            wchar_t* m_writePtr = nullptr;
            wchar_t* m_writeEnd = nullptr;

            LocalAbsolutePathBuilder(StringView<wchar_t> rootPath)
            {
                ASSERT(rootPath.length() < MAX_LENGTH);

                memcpy(m_path, rootPath.data(), rootPath.length() * sizeof(wchar_t));
                m_path[rootPath.length()] = 0;

                m_writePtr = m_path + rootPath.length();
                m_writeEnd = m_path + MAX_LENGTH;
            }

            StringView<wchar_t> view() const
            {
                return StringView<wchar_t>(m_path, m_writePtr);
            }

            const wchar_t* c_str() const
            {
                return m_path;
            }

            bool append(StringView<char> path)
            {
                if (m_writePtr + path.length() >= m_writeEnd)
                    return false;

                for (auto ch : path)
                {
                    if (ch == io::AbsolutePath::WRONG_SYSTEM_PATH_SEPARATOR)
                        ch = io::AbsolutePath::SYSTEM_PATH_SEPARATOR;
                    *m_writePtr++ = ch;
                }
                *m_writePtr = 0;

                return true;
            }

            bool appendAsDirName(StringView<char> path)
            {
                if (m_writePtr + path.length() >= m_writeEnd)
                    return false;

                for (auto ch : path)
                {
                    if (ch == io::AbsolutePath::WRONG_SYSTEM_PATH_SEPARATOR)
                        ch = io::AbsolutePath::SYSTEM_PATH_SEPARATOR;
                    *m_writePtr++ = ch;
                }

                const char pathSeparatorTxt[2] = { io::AbsolutePath::SYSTEM_PATH_SEPARATOR , 0 };
                if (!path.endsWith(pathSeparatorTxt))
                    *m_writePtr++ = io::AbsolutePath::SYSTEM_PATH_SEPARATOR;

                *m_writePtr = 0;
                return true;
            }
        };

        //--

        FileSystemNative::FileSystemNative(const io::AbsolutePath& rootPath, bool allowWrites, DepotStructure* owner)
            : m_rootPath(rootPath)
            , m_writable(allowWrites)
            , m_depot(owner)
        {
            // load stuff
            m_crcCache.create();

            // each native file system has a crc cache
            auto crcCacheName = StringBuf(TempString("fs_{}_crc.cache", rootPath.view().calcCRC64()));
            auto crcCachePath =  IO::GetInstance().systemPath(io::PathCategory::TempDir).addFile(crcCacheName.c_str());
            m_crcCache->load(crcCachePath);
        }

        FileSystemNative::~FileSystemNative()
        {
            if (m_tracker)
            {
                m_tracker->dettachListener(this);
                m_tracker.reset();
            }
        }

        bool FileSystemNative::isPhysical() const
        {
            return true;
        }

        bool FileSystemNative::isWritable() const
        {
            return m_writable;
        }

        bool FileSystemNative::ownsFile(StringView<char> rawFilePath) const
        {
            // native file system owns all files in the directory, no exceptions
            return true;
        }

        bool FileSystemNative::contextName(StringView<char> rawFilePath, StringBuf& outContextName) const
        {
            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.append(rawFilePath))
                return false;

            outContextName = StringBuf(path.c_str());
            return true;
        }

        bool FileSystemNative::info(StringView<char> rawFilePath, uint64_t* outCRC, uint64_t* outFileSize, io::TimeStamp* outTimestamp) const
        {
            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.append(rawFilePath))
                return false;

            if (nullptr != outCRC)
                return m_crcCache->fileCRC(path.view(), *outCRC, (uint64_t*)outTimestamp, outFileSize);
            else if (outTimestamp)
                return IO::GetInstance().fileTimeStamp(path.view(), *outTimestamp, outFileSize);
            else if (outFileSize)
                return IO::GetInstance().fileSize(path.view(), *outFileSize);
            else
                return true;
        }

        bool FileSystemNative::absolutePath(StringView<char> rawFilePath, io::AbsolutePath& outAbsolutePath) const
        {
            LocalAbsolutePathBuilder path(m_rootPath);
            if (path.append(rawFilePath))
            {
                outAbsolutePath = io::AbsolutePath::Build(path.view());
                return true;
            }

            return false;
        }

        bool FileSystemNative::enumDirectoriesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(StringView<char>)>& enumFunc) const
        {
            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.appendAsDirName(rawDirectoryPath))
                return false;

            return IO::GetInstance().findSubDirs(path.view(), [&enumFunc](StringView<wchar_t> name) {
                    if (name == L".boomer")
                        return false;
                    return enumFunc(TempString("{}", name));
                });
        }

        bool FileSystemNative::enumFilesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(StringView<char>)>& enumFunc) const
        {
            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.appendAsDirName(rawDirectoryPath))
                return false;

            return IO::GetInstance().findLocalFiles(path.view(), L"*.*", [&enumFunc](StringView<wchar_t> name) {
                return enumFunc(TempString("{}", name));
                });
        }

        io::FileHandlePtr FileSystemNative::createReader(StringView<char> rawFilePath) const
        {
            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.append(rawFilePath))
                return nullptr;

            // do not open if file does not exist
            if (rawFilePath.endsWith(".package") || rawFilePath.endsWith(".manifest") || rawFilePath.endsWith(".meta"))
                if (!IO::GetInstance().fileExists(path.view()))
                    return nullptr;

            // create a reader
            return IO::GetInstance().openForReading(path.view());
        }

        bool FileSystemNative::enableWriteOperations()
        {
            if (m_writable)
            {
                if (!m_writesEnabled)
                {
                    TRACE_WARNING("Enabled writes for file system at '{}'", m_rootPath);
                    m_writesEnabled = true;
                }
            }
            else
            {
                TRACE_ERROR("File system at '{}' is read-only", m_rootPath);
            }

            return m_writesEnabled;
        }

        void FileSystemNative::enableFileSystemObservers()
        {
            if (!m_tracker)
            {
                // create a watcher of file modifications
                m_tracker = IO::GetInstance().createDirectoryWatcher(m_rootPath);
                if (m_tracker)
                {
                    TRACE_INFO("Enabled live tracking of file system at '{}'", m_rootPath);
                    m_tracker->attachListener(this);
                }
                else
                {
                    TRACE_WARNING("Failed to start live tracking of file system at '{}'", m_rootPath);
                }
            }
        }

        bool FileSystemNative::writeFile(StringView<char> rawFilePath, const Buffer& data, const io::TimeStamp* overrideTimeStamp/*=nullptr*/, uint64_t overrideCRC/*=0*/)
        {
            ScopeTimer timer;

            ASSERT_EX(data, "No data specified");

            // translate path
            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.append(rawFilePath))
            {
                TRACE_ERROR("Unable to translate '{}' to a global file path", rawFilePath);
                return false;
            }

            // target file is read only, we can't save
            if (IO::GetInstance().isFileReadOnly(path.view()))
            {
                TRACE_ERROR("Unable to save '{}' as file is read only", rawFilePath);
                return false;
            }

            // save a .tmp file
            auto outputFileName = io::AbsolutePath::Build(path.view()).addExtension(".out");

            // delete existing output file
            if (IO::GetInstance().fileExists(outputFileName))
            {
                TRACE_WARNING("Output file '{}' already exists", outputFileName);
                if (!IO::GetInstance().deleteFile(outputFileName))
                {
                    TRACE_WARNING("Unable to save '{}': existing temporary output file exists and can't be deleted", rawFilePath);
                    return false;
                }
            }

			// open file for writing
            {
                auto writer = IO::GetInstance().openForWriting(outputFileName, false);
                if (writer)
                {
                    // save stuff to a temp file
                    auto numWritten = writer->writeSync(data.data(), data.size());
                    if (numWritten != data.size())
                    {
                        TRACE_ERROR("Unable to save '{}': written {} instead of {} bytes, is the disk full?", rawFilePath, numWritten, data.size());
                        return false;
                    }
                }
                else
                {
                    TRACE_ERROR("Unable to save '{}': failed to open target file, is the disk full?", rawFilePath);
                    return false;
                }
            }

            // delete previous backup
            auto backupFileName = io::AbsolutePath::Build(path.view()).addExtension(".bak");
            bool useBackFile = IO::GetInstance().fileExists(path.view());
            if (useBackFile)
            {
                if (IO::GetInstance().fileExists(backupFileName))
                {
                    if (!IO::GetInstance().deleteFile(backupFileName))
                    {
                        TRACE_WARNING("Unable to save '{}': existing backup file cannot be deleted, is the disk full?", rawFilePath);
                        IO::GetInstance().deleteFile(backupFileName);
                        IO::GetInstance().deleteFile(outputFileName);
                        useBackFile = false;
                    }
                }
            }

            // rename target file to backup file
            if (useBackFile)
            {
                if (!IO::GetInstance().moveFile(path.view(), backupFileName))
                {
                    TRACE_ERROR("Unable to save '{}': unable to create backup file, is the disk full?", rawFilePath);
                    return false;
                }
            }

            // move the output file to new place
            bool saved = true;
            if (!IO::GetInstance().moveFile(outputFileName, path.view()))
            {
                IO::GetInstance().deleteFile(outputFileName);
                TRACE_ERROR("Unable to save '{}': failed to move output file to target location, is the disk full?", rawFilePath);                
                saved = false;
            }

            // delete the backup file
            if (useBackFile)
            {
                if (saved)
                {
                    if (!IO::GetInstance().deleteFile(backupFileName))
                    {
                        TRACE_WARNING("Unable to delete backup file '{}', next save may fail", rawFilePath);
                    }
                }
                else
                {
                    IO::GetInstance().moveFile(backupFileName, path.view());
                }
            }

            // file saved
            TRACE_INFO("Saved '{}', {} in {}", path.view(), MemSize(data.size()), TimeInterval(timer.timeElapsed()));
            return saved;
        }

        //--

        static void NormalizeFilePath(UTF16StringBuf& path)
        {
            for (auto& ch : path)
            {
                if (ch == '\\')
                    ch = '/';
            }
        }

        static void NormalizeDirPath(UTF16StringBuf& path)
        {
            for (auto& ch : path)
            {
                if (ch == '\\')
                    ch = '/';
            }

            if (!path.endsWith(L"/"))
                path += "/";
        }

        void FileSystemNative::handleEvent(const io::DirectoryWatcherEvent& evt)
        {
            // ignore shit with backup files
            if (evt.path.view().endsWith(L".bak"))
                return;

            if (m_depot)
            {
                if (evt.type == io::DirectoryWatcherEventType::FileContentChanged ||
                    evt.type == io::DirectoryWatcherEventType::FileMetadataChanged)
                {
                    auto localPath = evt.path.relativeTo(m_rootPath);
                    NormalizeFilePath(localPath);
                    m_depot->notifyFileChanged(this, StringBuf(localPath.view()));
                }
                else if (evt.type == io::DirectoryWatcherEventType::FileAdded)
                {
                    auto localPath = evt.path.relativeTo(m_rootPath);
                    NormalizeFilePath(localPath);
                    m_depot->notifyFileAdded(this, StringBuf(localPath.view()));
                }
                else if (evt.type == io::DirectoryWatcherEventType::FileRemoved)
                {
                    auto localPath = evt.path.relativeTo(m_rootPath);
                    NormalizeFilePath(localPath);
                    m_depot->notifyFileRemoved(this, StringBuf(localPath.view()));
                }
                else if (evt.type == io::DirectoryWatcherEventType::DirectoryAdded)
                {
                    auto localPath = evt.path.relativeTo(m_rootPath);
                    NormalizeDirPath(localPath);
                    if (localPath != L".boomer")
                        m_depot->notifyDirAdded(this, StringBuf(localPath.view()));
                }
                else if (evt.type == io::DirectoryWatcherEventType::DirectoryRemoved)
                {
                    auto localPath = evt.path.relativeTo(m_rootPath);
                    NormalizeDirPath(localPath);
                    if (localPath != L".boomer")
                        m_depot->notifyDirRemoved(this, StringBuf(localPath.view()));
                }
            }
        }

        //--

    } // depot
} // base
