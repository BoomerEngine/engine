/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot #]
***/

#include "build.h"
#include "depotFileSystem.h"
#include "depotStructure.h"

#include "base/resource/include/resourceMetadata.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resource.h"
#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/inplaceArray.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/system/include/thread.h"

namespace base
{
    namespace depot
    {

        bool DepotStructure::queryFileMountPoint(StringView<char> fileSystemPath, res::ResourceMountPoint& outMountPoint) const
        {
            // look at mounted file systems looking for one that can service loading
            for (auto dep  : m_fileSystemsPtrs)
            {
                // last one, always matches
                if (dep->mountPoint().root())
                {
                    outMountPoint = res::ResourceMountPoint::ROOT();
                    return true;
                }

                // try match
                if (dep->mountPoint().containsPath(fileSystemPath))
                {
                    outMountPoint = dep->mountPoint();
                    return true;
                }
            }

            // not found
            return false;
        }

        bool DepotStructure::queryFilePlacement(StringView<char> fileSystemPath, const IFileSystem*& outFileSystem, StringBuf& outFileSystemPath, bool physicalOnly /*= false*/) const
        {
            // look at mounted file systems looking for one that can service loading
            for (auto dep  : m_fileSystemsPtrs)
            {
                // last one, always matches
                if (dep->mountPoint().root())
                {
                    outFileSystemPath = StringBuf(fileSystemPath);
                    outFileSystem = &dep->fileSystem();
                    return true;
                }

                // try match
                if (!physicalOnly || dep->fileSystem().isWritable())
                {
                    if (dep->mountPoint().translatePathToRelative(fileSystemPath, outFileSystemPath))
                    {
                        outFileSystem = &dep->fileSystem();
                        return true;
                    }
                }
            }

            // not found
            return false;
        }

        bool DepotStructure::queryContextName(StringView<char> fileSystemPath, StringBuf& contextName) const
        {
            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(fileSystemPath, localFileSystem, localFileSystemPath))
                return false;

            // get the timestamp
            return localFileSystem->contextName(localFileSystemPath, contextName);
        }

        bool DepotStructure::queryFileTimestamp(StringView<char> fileSystemPath, io::TimeStamp& outTimestamp) const
        {
            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(fileSystemPath, localFileSystem, localFileSystemPath))
                return false;

            // get the timestamp
            return localFileSystem->timestamp(localFileSystemPath, outTimestamp);
        }

        bool DepotStructure::queryFileAbsolutePath(StringView<char> fileSystemPath, io::AbsolutePath& outAbsolutePath) const
        {
            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!queryFilePlacement(fileSystemPath, localFileSystem, localFileSystemPath))
                return false;

            // get the timestamp
            return localFileSystem->absolutePath(localFileSystemPath, outAbsolutePath);
        }

        bool DepotStructure::queryFileDepotPath(const io::AbsolutePath& absolutePath, StringBuf& outFileSystemPath) const
        {
            StringBuf bestFileSystemPath;

            // look at mounted file systems looking for one that can service loading
            for (auto dep  : m_fileSystemsPtrs)
            {
                // get the absolute path for the mount point
                // NOTE: this works only if we are based on physical data
                io::AbsolutePath mountPointAbsolutePath;
                if (dep->fileSystem().absolutePath(StringBuf::EMPTY(), mountPointAbsolutePath))
                {
                    // get relative path
                    auto relativePath = absolutePath.relativeTo(mountPointAbsolutePath).ansi_str();
                    TRACE_INFO("Path '{}' resolved to '{}' relative to '{}'", absolutePath, relativePath, mountPointAbsolutePath);
                    if (!relativePath.empty() && (INDEX_NONE == relativePath.findStr("..")))
                    {
                        // use the shortest path (best mount point)
                        StringBuf fileSystemPath;
                        dep->mountPoint().expandPathFromRelative(relativePath, fileSystemPath);
                        if (bestFileSystemPath.empty() || fileSystemPath.length() < bestFileSystemPath.length())
                        {
                            bestFileSystemPath = fileSystemPath;
                        }
                    }
                }
            }

            if (!bestFileSystemPath.empty())
            {
                outFileSystemPath = bestFileSystemPath;
                return true;
            }

            return false;
        }

        bool DepotStructure::enumDirectoriesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(const DirectoryInfo& info)>& enumFunc) const
        {
            ASSERT_EX(rawDirectoryPath.empty() || rawDirectoryPath.endsWith("/"), TempString("Invalid raw directory path {}", rawDirectoryPath));

            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (queryFilePlacement(rawDirectoryPath, localFileSystem, localFileSystemPath))
            {
                // ask file system for directories
                if (localFileSystem->enumDirectoriesAtPath(localFileSystemPath, [localFileSystem, &enumFunc](StringView<char> dirName)
                    {
                        DirectoryInfo entry;
                        entry.fileSystem = localFileSystem;
                        entry.name = dirName;
                        entry.fileSystemRoot = false;
                        return enumFunc(entry);
                    }))
                    return true;
            }

            // look at the mounted packages at this directory
            auto fileSystemListAtDir  = m_fileSystemsAtDirectory.find(rawDirectoryPath);
            if (fileSystemListAtDir)
            {
                for (auto& fsEntry : *fileSystemListAtDir)
                {
                    DirectoryInfo entry;
                    entry.fileSystem = &fsEntry.fileSystem->fileSystem();
                    entry.name = fsEntry.name;
                    entry.fileSystemRoot = true;
                    if (enumFunc(entry))
                        return true;
                }
            }

			return false;
        }

        bool DepotStructure::enumFilesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(const FileInfo & info)>& enumFunc) const
        {
            ASSERT_EX(rawDirectoryPath.empty() || rawDirectoryPath.endsWith("/"), "Invalid raw directory path");

            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (queryFilePlacement(rawDirectoryPath, localFileSystem, localFileSystemPath))
            {
                // ask file system for directories
                return localFileSystem->enumFilesAtPath(localFileSystemPath, [&enumFunc, localFileSystem](StringView<char> fileName)
                    {
                        FileInfo entry;
                        entry.fileSystem = localFileSystem;
                        entry.name = fileName;
                        return enumFunc(entry);
                    });
            }

            return false;
        }

        void DepotStructure::enumAbsolutePathRoots(Array<io::AbsolutePath>& outAbsolutePathRoots) const
        {
            for (auto dep  : m_fileSystemsPtrs)
            {
                // get the absolute path for the mount point
                // NOTE: this works only if we are based on physical data
                io::AbsolutePath mountPointAbsolutePath;
                if (dep->fileSystem().absolutePath(StringBuf::EMPTY(), mountPointAbsolutePath))
                {
                    outAbsolutePathRoots.pushBack(mountPointAbsolutePath);
                }
            }
        }

    } // depot
} // base
