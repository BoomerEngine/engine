/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/io/include/crcCache.h"
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

        /// Depot file system observer
        class BASE_RESOURCE_COMPILER_API IDepotObserver : public base::NoCopy
        {
        public:
            virtual ~IDepotObserver();

            // file was changed
            virtual void notifyFileChanged(StringView<char> depotFilePath) {};

            // file was added
            virtual void notifyFileAdded(StringView<char> depotFilePath) {};

            // file was removed
            virtual void notifyFileRemoved(StringView<char> depotFilePath) {};

            // directory was added
            virtual void notifyDirAdded(StringView<char> depotFilePath) {};

            // directory was removed
            virtual void notifyDirRemoved(StringView<char> depotFilePath) {};
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

            /// get the general CRC cache, can be used for other depot related stuff
            INLINE io::CRCCache& crcCache() const { return *m_crcCache; }

            /// get the list of the mounted file systems
            INLINE const Array<const DepotFileSystem*>& mountedFileSystems() const { return m_fileSystemsPtrs; }

            //---

            /// initialize from given depot manifest (mapping file)
            bool populateFromManifest(const io::AbsolutePath& depotManifestPath);

            //---

            /// enable write operations on the depot, can fail if a file system does not permit that
            /// NOTE: this is necessary for the editor
            bool enableWriteOperations();

            /// enable live observation of changes in the file system, especially added/modified files
            void enableDepotObservers();

            //---

            /// attach a file system so we can load from it
            void attachFileSystem(StringView<char> prefixPath, UniquePtr<IFileSystem> fileSystem, DepotFileSystemType type);

            /// detach all project file systems
            void detachProjectFileSystems();
            
            //--

            /// attach observer
            void attachObserver(IDepotObserver* observer);

            /// detach observer
            void detttachObserver(IDepotObserver* observer);

            //--

            // given a resource path find a file system and local path for loading stuff
            // NOTE: this may fail if path is not recognized but usually it does not since the "/" is mapped as project root
            bool queryFilePlacement(StringView<char> fileSystemPath, const IFileSystem*& outFileSystem, StringBuf& outFileSystemPath, bool physicalOnly=false) const;

            /// get the context name for the given file
            bool queryContextName(StringView<char>fileSystemPath, StringBuf& contextName) const;

            /// get the file information
            bool queryFileInfo(StringView<char> fileSystemPath, uint64_t* outCRC, uint64_t* outFileSize, io::TimeStamp* outTimestamp) const;

            /// get the mount point path for given file system path
            bool queryFileMountPoint(StringView<char> fileSystemPath, res::ResourceMountPoint& outMountPoint) const;

            /// if the file is loaded from a physical file query it's path
            /// NOTE: is present only for files physically on disk, not in archives or over the network
            bool queryFileAbsolutePath(StringView<char> fileSystemPath, io::AbsolutePath& outAbsolutePath) const;

            /// if the given absolute file is loadable via the depot path than find it
            /// NOTE: is present only for files physically on disk, not in archives or over the network
            bool queryFileDepotPath(const io::AbsolutePath& absolutePath, StringBuf& outFileSystemPath) const;

            /// create a reader for the file's content
            /// NOTE: creating a reader may take some time
            io::FileHandlePtr createFileReader(StringView<char> fileSystemPath) const;

            /// store new content for a file
            /// NOTE: fails if the file system was not writable
            bool storeFileContent(StringView<char> filePath, const Buffer& newContent) const;

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

            /// get files at given path
            bool enumFilesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(const FileInfo& info)>& enumFunc) const;

            //--

            /// enumerate all physical file system roots we have (so we know what to observe)
            void enumAbsolutePathRoots(Array<io::AbsolutePath>& outAbsolutePathRoots) const;

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
            UniquePtr<io::CRCCache> m_crcCache;
            io::AbsolutePath m_cacheDirectory;

            // file systems
            Array<UniquePtr<DepotFileSystem>> m_fileSystems;
            Array<const DepotFileSystem*> m_fileSystemsPtrs;

            // all loaded manifests
            Array<PackageManifestPtr> m_manifests;

            // depot observers
            Mutex m_observersLock;
            MutableArray<IDepotObserver*> m_observers;

            struct DepotFileSystemBindingInfo
            {
                StringBuf name;
                const DepotFileSystem* fileSystem = nullptr;
            };

            HashMap<StringBuf, Array<DepotFileSystemBindingInfo>> m_fileSystemsAtDirectory;

            Array<res::ResourcePath> m_resourcesToReload;
            SpinLock m_resourcesToReloadLock;

            //---

            void rebuildFileSystemMap();
            void registerFileSystemBinding(const DepotFileSystem* fs);


            void unloadProject();
            bool processFileSystem(StringView<char> mountPath, UniquePtr<IFileSystem> fs, DepotFileSystemType type);

            //---
        };

    } // depot
} // base