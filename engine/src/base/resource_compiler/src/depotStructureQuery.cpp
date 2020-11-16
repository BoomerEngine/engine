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
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(fileSystemPath, DepotPathClass::AnyAbsolutePath), false);

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
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(fileSystemPath, DepotPathClass::AbsoluteFilePath), false);
            return translatePath(fileSystemPath, outFileSystem, outFileSystemPath, physicalOnly);
        }

        bool DepotStructure::translatePath(StringView<char> fileSystemPath, const IFileSystem*& outFileSystem, StringBuf& outFileSystemPath, bool physicalOnly /*= false*/) const
        {
            // look at mounted file systems looking for one that can service loading
            for (auto dep : m_fileSystemsPtrs)
            {
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
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(fileSystemPath, DepotPathClass::AbsoluteFilePath), false);

            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!translatePath(fileSystemPath, localFileSystem, localFileSystemPath))
                return false;

            // get the timestamp
            return localFileSystem->contextName(localFileSystemPath, contextName);
        }

        bool DepotStructure::queryFileTimestamp(StringView<char> fileSystemPath, io::TimeStamp& outTimestamp) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(fileSystemPath, DepotPathClass::AbsoluteFilePath), false);

            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!translatePath(fileSystemPath, localFileSystem, localFileSystemPath))
                return false;

            // get the timestamp
            return localFileSystem->timestamp(localFileSystemPath, outTimestamp);
        }

        bool DepotStructure::queryFileAbsolutePath(StringView<char> fileSystemPath, StringBuf& outAbsolutePath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(fileSystemPath, DepotPathClass::AnyAbsolutePath), false);

            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (!translatePath(fileSystemPath, localFileSystem, localFileSystemPath))
                return false;

            // get the timestamp
            return localFileSystem->absolutePath(localFileSystemPath, outAbsolutePath);
        }

        bool DepotStructure::queryFileDepotPath(StringView<char> absolutePath, StringBuf& outFileSystemPath) const
        {
            StringBuf bestFileSystemPath;

            // look at mounted file systems looking for one that can service loading
            for (auto dep  : m_fileSystemsPtrs)
            {
                // get the absolute path for the mount point
                // NOTE: this works only if we are based on physical data
                StringBuf mountPointAbsolutePath;
                if (dep->fileSystem().absolutePath(StringBuf::EMPTY(), mountPointAbsolutePath))
                {
                    // get relative path
                    auto relativePath = absolutePath.afterFirst(mountPointAbsolutePath);
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
                ASSERT(ValidateDepotPath(bestFileSystemPath, DepotPathClass::AbsoluteFilePath));
                outFileSystemPath = bestFileSystemPath;
                return true;
            }

            return false;
        }

        bool DepotStructure::enumDirectoriesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(const DirectoryInfo& info)>& enumFunc) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawDirectoryPath, DepotPathClass::AbsoluteDirectoryPath), false);

            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (translatePath(rawDirectoryPath, localFileSystem, localFileSystemPath))
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
            if (auto fileSystemListAtDir = m_fileSystemsAtDirectory.find(rawDirectoryPath))
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
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(rawDirectoryPath, DepotPathClass::AbsoluteDirectoryPath), false);

            // resolve location
            StringBuf localFileSystemPath;
            const IFileSystem* localFileSystem = nullptr;
            if (translatePath(rawDirectoryPath, localFileSystem, localFileSystemPath))
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

        void DepotStructure::enumAbsolutePathRoots(Array<StringBuf>& outAbsolutePathRoots) const
        {
            for (auto dep  : m_fileSystemsPtrs)
            {
                // get the absolute path for the mount point
                // NOTE: this works only if we are based on physical data
                StringBuf mountPointAbsolutePath;
                if (dep->fileSystem().absolutePath(StringBuf::EMPTY(), mountPointAbsolutePath))
                {
                    outAbsolutePathRoots.pushBack(mountPointAbsolutePath);
                }
            }
        }

        struct DepotPathAppendBuffer
        {
            static const uint32_t MAX_LENGTH = 512;

            char m_buffer[MAX_LENGTH+1]; // NOTE: not terminated with zero while building, only at the end
            uint32_t m_pos = 0;

            DepotPathAppendBuffer()
            {}

            ALWAYS_INLINE bool empty() const
            {
                return m_pos == 0;
            }

            ALWAYS_INLINE StringView<char> view() const
            {
                return StringView<char>(m_buffer, m_pos);
            }

            bool append(StringView<char> text, uint32_t& outPos)
            {
                DEBUG_CHECK_RETURN_V(m_pos + text.length() <= MAX_LENGTH, false); // should not happen in healthy system

                outPos = m_pos;

                memcpy(m_buffer + m_pos, text.data(), text.length());
                m_pos += text.length();
                return true;
            }

            void revert(uint32_t pos)
            {
                m_pos = pos;
            }
        };


        bool DepotStructure::findFile(StringView<char> depotPath, StringView<char> fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const
        {
            DEBUG_CHECK_RETURN_V(ValidateDepotPath(depotPath, DepotPathClass::AbsoluteDirectoryPath), false);
            DEBUG_CHECK_RETURN_V(ValidateFileNameWithExtension(fileName), false);

            uint32_t pos = 0;
            DepotPathAppendBuffer path;
            DEBUG_CHECK_RETURN_V(path.append(depotPath, pos), false); // handle case when path is to long

            for (auto dep : m_fileSystemsPtrs)
                if (depotPath.beginsWith(dep->mountPoint().view()))
                    if (findFileInternal(dep, path, fileName, maxDepth, outFoundFileDepotPath))
                        return true;

            return findFileInternal(path, fileName, maxDepth, outFoundFileDepotPath);
        }

        bool DepotStructure::findFileInternal(DepotPathAppendBuffer& path, StringView<char> fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const
        {
            if (maxDepth > 0)
            {
                if (auto fileSystemListAtDir = m_fileSystemsAtDirectory.find(path.view()))
                {
                    for (auto& fsEntry : *fileSystemListAtDir)
                    {
                        uint32_t prevPos = 0;
                        if (path.append(TempString("{}/", fsEntry.name), prevPos))
                        {
                            if (fsEntry.fileSystem)
                            {
                                ASSERT(path.view().beginsWith(fsEntry.fileSystem->mountPoint().view()));
                                if (findFileInternal(fsEntry.fileSystem, path, fileName, maxDepth - 1, outFoundFileDepotPath))
                                    return true;
                            }
                            else
                            {
                                if (findFileInternal(path, fileName, maxDepth - 1, outFoundFileDepotPath))
                                    return true;
                            }

                            path.revert(prevPos);
                        }
                    }
                }
            }

            return false;
        }

        bool DepotStructure::findFileInternal(const DepotFileSystem* fs, DepotPathAppendBuffer& path, StringView<char> matchFileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const
        {
            const auto mountPoint = fs->mountPoint().view();
            const auto relativePath = path.view().subString(mountPoint.length());

            if (fs->fileSystem().enumFilesAtPath(relativePath, [&outFoundFileDepotPath, &path, matchFileName](StringView<char> name)
                {
                    if (name.caseCmp(matchFileName) == 0)
                    {
                        outFoundFileDepotPath = TempString("{}{}", path.view(), name);
                        return true;
                    }

                    return false;
                }))
                return true;

            if (maxDepth > 0)
            {
                maxDepth -= 1;

                if (fs->fileSystem().enumDirectoriesAtPath(relativePath, [this, fs, &outFoundFileDepotPath, &path, matchFileName, maxDepth](StringView<char> name)
                    {
                        uint32_t pos = 0;
                        if (path.append(TempString("{}/", name), pos))
                        {
                            if (findFileInternal(fs, path, matchFileName, maxDepth, outFoundFileDepotPath))
                                return true;

                            path.revert(pos);
                        }

                        return false;
                    }))
                    return true;
            }

            return false;
        }

    } // depot
} // base
