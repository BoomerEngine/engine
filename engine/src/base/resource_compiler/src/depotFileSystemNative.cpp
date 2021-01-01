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
#include "base/containers/include/inplaceArray.h"
#include "base/containers/include/utf8StringFunctions.h"
#include "base/resource/include/resourceKey.h"

namespace base
{
    namespace depot
    {

        //--

        FileSystemNative::FileSystemNative(StringView rootPath, bool allowWrites, DepotStructure* owner)
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

        bool FileSystemNative::ownsFile(StringView rawFilePath) const
        {
            // native file system owns all files in the directory, no exceptions
            return true;
        }

        bool FileSystemNative::contextName(StringView rawFilePath, StringBuf& outContextName) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            StringBuilder path;
            path << m_rootPath;
            path << rawFilePath;

            outContextName = path.toString();
            return true;
        }

        bool FileSystemNative::timestamp(StringView rawFilePath, io::TimeStamp& outTimestamp) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            StringBuilder path;
            path << m_rootPath;
            path << rawFilePath;

            return base::io::FileTimeStamp(path.view(), outTimestamp);
        }

        bool FileSystemNative::absolutePath(StringView rawFilePath, StringBuf& outAbsolutePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::AnyRelativePath), false);

            StringBuilder path;
            path << m_rootPath;
            path << rawFilePath;

            outAbsolutePath = path.toString();
            return true;
        }

        bool FileSystemNative::enumDirectoriesAtPath(StringView rawDirectoryPath, const std::function<bool(StringView)>& enumFunc) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawDirectoryPath, DepotPathClass::RelativeDirectoryPath), false);

            StringBuilder path;
            path << m_rootPath;
            path << rawDirectoryPath;
            path << "/";

            return base::io::FindSubDirs(path.view(), [&enumFunc](StringView name) {
                    if (name != ".boomer")
                        return enumFunc(name);
                    else
                        return false;
                });
        }

        bool FileSystemNative::enumFilesAtPath(StringView rawDirectoryPath, const std::function<bool(StringView)>& enumFunc) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawDirectoryPath, DepotPathClass::RelativeDirectoryPath), false);

            StringBuilder path;
            path << m_rootPath;
            path << rawDirectoryPath;
            path << "/";

            return base::io::FindLocalFiles(path.view(), "*.*", [&enumFunc](StringView name) {
                return enumFunc(TempString("{}", name));
                });
        }

        io::ReadFileHandlePtr FileSystemNative::createReader(StringView rawFilePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            StringBuilder path;
            path << m_rootPath;
            path << rawFilePath;

            // do not open if file does not exist
            if (!base::io::FileExists(path.view()))
                return nullptr;

            // create a reader
            return base::io::OpenForReading(path.view());
        }

        io::AsyncFileHandlePtr FileSystemNative::createAsyncReader(StringView rawFilePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            StringBuilder path;
            path << m_rootPath;
            path << rawFilePath;

            // do not open if file does not exist
            if (!base::io::FileExists(path.view()))
                return nullptr;

            // create a reader
            return base::io::OpenForAsyncReading(path.view());
        }

        io::WriteFileHandlePtr FileSystemNative::createWriter(StringView rawFilePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawFilePath, DepotPathClass::RelativeFilePath), false);

            StringBuilder path;
            path << m_rootPath;
            path << rawFilePath;

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

        static void NormalizeFilePath(StringBuf& path)
        {
            path.replaceChar('\\', '/');
        }

        static void NormalizeDirPath(StringBuf& path)
        {
            path.replaceChar('\\', '/');

            if (!path.endsWith("/"))
                path = TempString("{}/", path);
        }

        void FileSystemNative::handleEvent(const io::DirectoryWatcherEvent& evt)
        {
            // ignore shit with backup files
            if (evt.path.view().endsWith(".bak"))
                return;

            if (m_depot)
            {
                if (evt.type == io::DirectoryWatcherEventType::FileContentChanged ||
                    evt.type == io::DirectoryWatcherEventType::FileMetadataChanged)
                {
                    if (ValidateDepotPath(evt.path, DepotPathClass::AbsoluteFilePath))
                    {
                        auto localPath = evt.path.stringAfterFirst(m_rootPath);
                        if (!localPath.empty())
                        {
                            NormalizeFilePath(localPath);
                            m_depot->notifyFileChanged(this, localPath);
                        }
                    }
                }
                else if (evt.type == io::DirectoryWatcherEventType::FileAdded)
                {
                    if (ValidateDepotPath(evt.path, DepotPathClass::AbsoluteFilePath))
                    {
                        auto localPath = evt.path.stringAfterFirst(m_rootPath);
                        if (!localPath.empty())
                        {
                            NormalizeFilePath(localPath);
                            m_depot->notifyFileAdded(this, localPath);
                        }
                    }
                }
                else if (evt.type == io::DirectoryWatcherEventType::FileRemoved)
                {
                    if (ValidateDepotPath(evt.path, DepotPathClass::AbsoluteFilePath))
                    {
                        auto localPath = evt.path.stringAfterFirst(m_rootPath);
                        if (!localPath.empty())
                        {
                            NormalizeFilePath(localPath);
                            m_depot->notifyFileRemoved(this, localPath);
                        }
                    }
                }
                else if (evt.type == io::DirectoryWatcherEventType::DirectoryAdded)
                {
                    if (ValidateDepotPath(evt.path, DepotPathClass::AbsoluteDirectoryPath))
                    {
                        auto localPath = evt.path.stringAfterFirst(m_rootPath);
                        if (!localPath.empty())
                        {
                            NormalizeDirPath(localPath);
                            m_depot->notifyDirAdded(this, localPath);
                        }
                    }
                }
                else if (evt.type == io::DirectoryWatcherEventType::DirectoryRemoved)
                {
                    if (ValidateDepotPath(evt.path, DepotPathClass::AbsoluteDirectoryPath))
                    {
                        auto localPath = evt.path.stringAfterFirst(m_rootPath);
                        if (!localPath.empty())
                        {
                            NormalizeDirPath(localPath);
                            m_depot->notifyDirRemoved(this, localPath);
                        }
                    }
                }
            }
        }

        //--

    } // depot
} // base
