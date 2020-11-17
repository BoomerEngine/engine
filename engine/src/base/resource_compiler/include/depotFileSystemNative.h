/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot\filesystem #]
***/

#pragma once

#include "depotFileSystem.h"
#include "base/io/include/ioDirectoryWatcher.h"

namespace base
{
    namespace depot
    {

        /// Raw file system for resources based that sucks data from physical folder on disk
        class BASE_RESOURCE_COMPILER_API FileSystemNative : public IFileSystem, public io::IDirectoryWatcherListener
        {
        public:
            FileSystemNative(StringView rootPath, bool allowWrites, DepotStructure* owner);
            virtual ~FileSystemNative();

            /// get the file path
            INLINE StringView rootPath() const { return m_rootPath; }

            /// IRawFileSystem
            virtual bool isPhysical() const override final;
            virtual bool isWritable() const override final;
            virtual bool ownsFile(StringView rawFilePath) const override final;
            virtual bool contextName(StringView rawFilePath, StringBuf& outContextName) const override final;
            virtual bool timestamp(StringView rawFilePath, io::TimeStamp& outTimestamp) const override final;
            virtual bool absolutePath(StringView rawFilePath, StringBuf& outAbsolutePath) const override final;

            // Enum
            virtual bool enumDirectoriesAtPath(StringView rawDirectoryPath, const std::function<bool(StringView)>& enumFunc) const override final;
            virtual bool enumFilesAtPath(StringView rawDirectoryPath, const std::function<bool(StringView)>& enumFunc) const override final;

            // IO
            virtual io::ReadFileHandlePtr createReader(StringView rawFilePath) const override final;
            virtual io::WriteFileHandlePtr createWriter(StringView rawFilePath) const override final;
            virtual io::AsyncFileHandlePtr createAsyncReader(StringView rawFilePath) const override final;

            // Editor support
            virtual void enableFileSystemObservers() override final;

        private:
            StringBuf m_rootPath;

            io::DirectoryWatcherPtr m_tracker;
            bool m_writable = false;

            DepotStructure* m_depot;

            /// io::IDirectoryWatcherListener
            virtual void handleEvent(const io::DirectoryWatcherEvent& evt) override final;
        };

    } // depot
} // base