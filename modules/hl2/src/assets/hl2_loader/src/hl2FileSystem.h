/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#pragma once

#include "base/resource_compiler/include/depotFileSystem.h"
#include "base/io/include/ioFileHandle.h"

namespace hl2
{

    class FileSystemIndex;

    /// file system wrapper for HL2
    class ASSETS_HL2_LOADER_API PackedFileSystem : public base::depot::IFileSystem
    {
    public:
        PackedFileSystem(const base::StringBuf& rootPath, base::UniquePtr<FileSystemIndex>&& indexData);
        virtual ~PackedFileSystem();

        /// base::depot::IFileSystem interface
        virtual bool isPhysical() const override final;
        virtual bool isWritable() const override final;
        virtual bool ownsFile(StringView rawFilePath) const override final;
        virtual bool contextName(StringView rawFilePath, base::StringBuf& outContextName) const override final;
        virtual bool timestamp(StringView rawFilePath, base::io::TimeStamp& outTimestamp) const override final;
        virtual bool absolutePath(StringView rawFilePath, base::StringBuf& outAbsolutePath) const override final;
        virtual bool enumDirectoriesAtPath(StringView rawDirectoryPath, const std::function<bool(StringView)>& enumFunc) const override final;
        virtual bool enumFilesAtPath(StringView rawDirectoryPath, const std::function<bool(StringView)>& enumFunc) const override final;
        //virtual base::io::FileHandlePtr createReader(StringView rawFilePath) const override final;
        //virtual bool writeFile(StringView awFilePath, const base::Buffer& data, const base::io::TimeStamp* overrideTimeStamp=nullptr, uint64_t overrideCRC=0) override final;
        //virtual bool enableWriteOperations() override final;
        virtual void enableFileSystemObservers() override final;

    private:
        base::StringBuf m_rootPath;
        base::UniquePtr<FileSystemIndex> m_indexData;

        struct OpenPackage
        {
            base::StringBuf m_fullPath;
            base::io::ReadFileHandlePtr m_fileHandle;
            mutable base::Mutex m_lock;
        };

        base::Array<OpenPackage> m_openedPackages;

        void setupPackages();
    };

} // hl2