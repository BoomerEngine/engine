/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot #]
***/

#pragma once

#include "base/system/include/atomic.h"
#include "base/resource/include/resourceLoader.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceMountPoint.h"
#include "depotFileSystem.h"

namespace base
{
    namespace depot
    {
        //---

        class TrackedDepot;
        class DepotStructureTrackingDepotIntegration;
        struct DepotPathAppendBuffer;

        //---

        // mounted depot's file systems
        class BASE_RESOURCE_COMPILER_API DepotFileSystem : public base::NoCopy
        {
        public:
            DepotFileSystem(const res::ResourceMountPoint& mountPoint, UniquePtr<IFileSystem> fileSystem, DepotFileSystemType type);
            ~DepotFileSystem();

            // prefix path in the depot, must end in / or be empty for root
            INLINE const res::ResourceMountPoint& mountPoint() const { return m_mountPoint; }

            // file system handler
            INLINE IFileSystem& fileSystem() const { return *m_fileSystem; }

            // type of the file system
            INLINE DepotFileSystemType type() const { return m_type; }

        private:
            // prefix path in the depot, must end in / or be empty for root
            res::ResourceMountPoint m_mountPoint;

            // file system handler
            UniquePtr<IFileSystem> m_fileSystem;

            // type of the file system
            DepotFileSystemType m_type;
        };

        //---

        /// helper class for managing depot structure of the project
        /// NOTE: depot files are RAW files from which we can load resources
        class BASE_RESOURCE_COMPILER_API DepotStructure : public NoCopy
        {
        public:
            DepotStructure();
            virtual ~DepotStructure();

            //---
            
            /// get the event key for this depot structure, allows us to listen to changes
            INLINE const GlobalEventKey& eventKey() const { return m_eventKey; }

            /// get the list of the mounted file systems
            INLINE const Array<const DepotFileSystem*>& mountedFileSystems() const { return m_fileSystemsPtrs; }

            //---

            /// initialize from given depot manifest (mapping file)
            bool populateFromManifest(StringView<char> depotManifestPath);

            //---

            /// enable live observation of changes in the file system, especially added/modified files
            void enableDepotObservers();

            //---

            /// attach a file system as a particular directory in the root (note: no nested path support!)
            void attachFileSystem(const res::ResourceMountPoint& mountPoint, UniquePtr<IFileSystem> fileSystem, DepotFileSystemType type);

            /// detach all project file systems
            void detachProjectFileSystems();
            
            //--

            /// get the context name for the given file
            bool queryContextName(StringView<char>fileSystemPath, StringBuf& contextName) const;

            // given a resource path find a file system and local path for loading stuff
            // NOTE: this may fail if path is not recognized but usually it does not since the "/" is mapped as project root
            bool queryFilePlacement(StringView<char> fileSystemPath, const IFileSystem*& outFileSystem, StringBuf& outFileSystemPath, bool physicalOnly=false) const;

            /// get the file information
            bool queryFileTimestamp(StringView<char> fileSystemPath, io::TimeStamp& outTimestamp) const;

            /// get the mount point path for given file system path
            bool queryFileMountPoint(StringView<char> fileSystemPath, res::ResourceMountPoint& outMountPoint) const;

            /// if the file is loaded from a physical file query it's path
            /// NOTE: is present only for files physically on disk, not in archives or over the network
            bool queryFileAbsolutePath(StringView<char> fileSystemPath, StringBuf& outAbsolutePath) const;

            /// if the given absolute file is loadable via the depot path than find it
            /// NOTE: is present only for files physically on disk, not in archives or over the network
            bool queryFileDepotPath(StringView<char> absolutePath, StringBuf& outFileSystemPath) const;

            /// create a reader for the file's content
            /// NOTE: creating a reader may take some time
            io::ReadFileHandlePtr createFileReader(StringView<char> filePath) const;

            /// create a writer for the file's content
            /// NOTE: fails if the file system was not writable
            io::WriteFileHandlePtr createFileWriter(StringView<char> filePath) const;

            /// create an ASYNC reader for the file's content
            /// NOTE: creating a reader may take some time
            io::AsyncFileHandlePtr createFileAsyncReader(StringView<char> filePath) const;

            //--

            struct DirectoryInfo
            {
                StringView<char> name;
                const IFileSystem* fileSystem = nullptr;
                bool fileSystemRoot = false;

                INLINE bool operator<(const DirectoryInfo& other) const
                {
                    return name < other.name;
                }
            };

            /// get child directories at given path
            /// NOTE: virtual directories for the mounted file systems are reported as well
            bool enumDirectoriesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(const DirectoryInfo& info) > & enumFunc) const;

            struct FileInfo
            {
                StringView<char> name;
                const IFileSystem* fileSystem = nullptr;

                INLINE bool operator<(const FileInfo& other) const
                {
                    return name < other.name;
                }
            };

            /// get files at given path (should be aboslute depot DIRECTORY path)
            bool enumFilesAtPath(StringView<char> path, const std::function<bool(const FileInfo& info)>& enumFunc) const;

            //--

            /// enumerate all physical file system roots we have (so we know what to observe)
            void enumAbsolutePathRoots(Array<StringBuf>& outAbsolutePathRoots) const;

            //--

            // find file in depot
            bool findFile(StringView<char> depotPath, StringView<char> fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const;

            //--

            // notify depot that a file in a given files system has changed content
            void notifyFileChanged(IFileSystem* fs, StringView<char> rawFilePath);

            // notify depot that a file was added to the given file system
            void notifyFileAdded(IFileSystem* fs, StringView<char> rawFilePath);

            // notify depot that a file was removed from the given file system
            void notifyFileRemoved(IFileSystem* fs, StringView<char> rawFilePath);

            // notify depot that a directory was added to the given file system
            void notifyDirAdded(IFileSystem* fs, StringView<char> rawFilePath);

            // notify depot that a directory was removed from the given file system
            void notifyDirRemoved(IFileSystem* fs, StringView<char> rawFilePath);

        private:
            // global event notifications
            GlobalEventKey m_eventKey;
                
            // file systems
            Array<UniquePtr<DepotFileSystem>> m_fileSystems;
            Array<const DepotFileSystem*> m_fileSystemsPtrs;

            struct DepotFileSystemBindingInfo
            {
                StringBuf name;
                const DepotFileSystem* fileSystem = nullptr;
            };

            HashMap<StringBuf, Array<DepotFileSystemBindingInfo>> m_fileSystemsAtDirectory;

            Array<StringBuf> m_resourcesToReload;
            SpinLock m_resourcesToReloadLock;

            //---

            void rebuildFileSystemMap();
            void registerFileSystemBinding(const DepotFileSystem* fs);

            bool translatePath(StringView<char> fileSystemPath, const IFileSystem*& outFileSystem, StringBuf& outFileSystemPath, bool physicalOnly = false) const;

            bool processFileSystem(const res::ResourceMountPoint& mountPoint, UniquePtr<IFileSystem> fs, DepotFileSystemType type);

            bool findFileInternal(DepotPathAppendBuffer& path, StringView<char> fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const;
            bool findFileInternal(const DepotFileSystem* fs, DepotPathAppendBuffer& path, StringView<char> fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const;

            //---
        };

        //--

    } // depot
} // base