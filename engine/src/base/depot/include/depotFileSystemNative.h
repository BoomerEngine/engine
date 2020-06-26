/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot\filesystem #]
***/

#pragma once

#include "depotFileSystem.h"
#include "base/io/include/absolutePath.h"
#include "base/io/include/ioDirectoryWatcher.h"

namespace base
{
    namespace depot
    {

        /// Raw file system for resources based that sucks data from physical folder on disk
        class BASE_DEPOT_API FileSystemNative : public IFileSystem, public io::IDirectoryWatcherListener
        {
        public:
            FileSystemNative(const io::AbsolutePath& rootPath, bool allowWrites, DepotStructure* owner);
            virtual ~FileSystemNative();

            /// get the file path
            INLINE const io::AbsolutePath& rootPath() const { return m_rootPath; }

            /// IRawFileSystem
            virtual bool isPhysical() const override final;
            virtual bool isWritable() const override final;
            virtual bool ownsFile(StringView<char> rawFilePath) const override final;
            virtual bool contextName(StringView<char> rawFilePath, StringBuf& outContextName) const override final;
            virtual bool info(StringView<char> rawFilePath, uint64_t* outCRC, uint64_t* outFileSize, io::TimeStamp* outTimestamp) const override final;
            virtual bool absolutePath(StringView<char> rawFilePath, io::AbsolutePath& outAbsolutePath) const override final;

            // Enum
            virtual bool enumDirectoriesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(StringView<char>)>& enumFunc) const override final;
            virtual bool enumFilesAtPath(StringView<char> rawDirectoryPath, const std::function<bool(StringView<char>)>& enumFunc) const override final;

            // IO
            virtual io::FileHandlePtr createReader(StringView<char> rawFilePath) const override final;

            // Editor support
            virtual bool writeFile(StringView<char> rawFilePath, const Buffer& data, const io::TimeStamp* overrideTimeStamp = nullptr, uint64_t overrideCRC = 0) override final;
            virtual bool enableWriteOperations() override final;
            virtual void enableFileSystemObservers() override final;

        private:
            io::AbsolutePath m_rootPath;
            UniquePtr<io::CRCCache> m_crcCache;
            io::DirectoryWatcherPtr m_tracker;
            bool m_writable = false;;
            bool m_writesEnabled = false;

            DepotStructure* m_depot;

            /// io::IDirectoryWatcherListener
            virtual void handleEvent(const io::DirectoryWatcherEvent& evt) override final;
        };

    } // depot
} // base