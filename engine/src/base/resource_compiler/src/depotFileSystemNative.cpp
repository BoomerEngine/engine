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
#include "base/io/include/absolutePath.h"
#include "base/containers/include/inplaceArray.h"
#include "base/containers/include/utf8StringFunctions.h"
#include "base/resource/include/resourcePath.h"

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

                /*ASSERT(m_path[rootPath.length() - 1] == io::AbsolutePath::SYSTEM_PATH_SEPARATOR);
                m_path[rootPath.length() - 1] = 0;*/

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
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.append(rawFilePath))
                return false;

            outContextName = StringBuf(path.c_str());
            return true;
        }

        bool FileSystemNative::timestamp(StringView<char> rawFilePath, io::TimeStamp& outTimestamp) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.append(rawFilePath))
                return false;

            return base::io::FileTimeStamp(path.view(), outTimestamp);
        }

        bool FileSystemNative::absolutePath(StringView<char> rawFilePath, io::AbsolutePath& outAbsolutePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::AnyRelativePath), false);

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
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawDirectoryPath, DepotPathClass::RelativeDirectoryPath), false);

            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.appendAsDirName(rawDirectoryPath))
                return false;

            return base::io::FindSubDirs(path.view(), [&enumFunc](StringView<wchar_t> name) {
                    if (name == L".boomer")
                        return false;
                    return enumFunc(TempString("{}", name));
                });
        }

        bool FileSystemNative::enumFilesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(StringView<char>)>& enumFunc) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawDirectoryPath, DepotPathClass::RelativeDirectoryPath), false);

            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.appendAsDirName(rawDirectoryPath))
                return false;

            return base::io::FindLocalFiles(path.view(), L"*.*", [&enumFunc](StringView<wchar_t> name) {
                return enumFunc(TempString("{}", name));
                });
        }

        io::ReadFileHandlePtr FileSystemNative::createReader(StringView<char> rawFilePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.append(rawFilePath))
                return nullptr;

            // do not open if file does not exist
            if (!base::io::FileExists(path.view()))
                return nullptr;

            // create a reader
            return base::io::OpenForReading(path.view());
        }

        io::AsyncFileHandlePtr FileSystemNative::createAsyncReader(StringView<char> rawFilePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.append(rawFilePath))
                return nullptr;

            // do not open if file does not exist
            if (!base::io::FileExists(path.view()))
                return nullptr;

            // create a reader
            return base::io::OpenForAsyncReading(path.view());
        }

        io::WriteFileHandlePtr FileSystemNative::createWriter(StringView<char> rawFilePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            LocalAbsolutePathBuilder path(m_rootPath);
            if (!path.append(rawFilePath))
                return nullptr;

            // create a STAGED writer
            const auto writeMode = io::FileWriteMode::StagedWrite;
            return base::io::OpenForWriting(path.view(), writeMode);
        }

        void FileSystemNative::enableFileSystemObservers()
        {
            if (!m_tracker)
            {
                // create a watcher of file modifications
                m_tracker = base::io::CreateDirectoryWatcher(m_rootPath);
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
                    if (ValidateDepotPath(evt.path.ansi_str(), DepotPathClass::AbsoluteFilePath))
                    {
                        auto localPath = evt.path.relativeTo(m_rootPath);
                        NormalizeFilePath(localPath);
                        m_depot->notifyFileChanged(this, StringBuf(localPath.view()));
                    }
                }
                else if (evt.type == io::DirectoryWatcherEventType::FileAdded)
                {
                    if (ValidateDepotPath(evt.path.ansi_str(), DepotPathClass::AbsoluteFilePath))
                    {
                        auto localPath = evt.path.relativeTo(m_rootPath);
                        NormalizeFilePath(localPath);
                        m_depot->notifyFileAdded(this, StringBuf(localPath.view()));
                    }
                }
                else if (evt.type == io::DirectoryWatcherEventType::FileRemoved)
                {
                    if (ValidateDepotPath(evt.path.ansi_str(), DepotPathClass::AbsoluteFilePath))
                    {
                        auto localPath = evt.path.relativeTo(m_rootPath);
                        NormalizeFilePath(localPath);
                        m_depot->notifyFileRemoved(this, StringBuf(localPath.view()));
                    }
                }
                else if (evt.type == io::DirectoryWatcherEventType::DirectoryAdded)
                {
                    if (ValidateDepotPath(evt.path.ansi_str(), DepotPathClass::AbsoluteDirectoryPath))
                    {
                        auto localPath = evt.path.relativeTo(m_rootPath);
                        NormalizeDirPath(localPath);
                        m_depot->notifyDirAdded(this, StringBuf(localPath.view()));
                    }
                }
                else if (evt.type == io::DirectoryWatcherEventType::DirectoryRemoved)
                {
                    if (ValidateDepotPath(evt.path.ansi_str(), DepotPathClass::AbsoluteDirectoryPath))
                    {
                        auto localPath = evt.path.relativeTo(m_rootPath);
                        NormalizeDirPath(localPath);
                        m_depot->notifyDirRemoved(this, StringBuf(localPath.view()));
                    }
                }
            }
        }

        //--

    } // depot
} // base