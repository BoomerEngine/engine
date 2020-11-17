/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\posix #]
* [#platform: posix #]
***/

#pragma once

#include "ioSystem.h"
#include "ioSystemHandler.h"
#include "ioFileHandle.h"

namespace base
{
    namespace io
    {
        namespace prv
        {

            class POSIXAsyncReadDispatcher;

            // the IO system implementation for Linux based systems
            class POSIXIOSystem : public ISystemHandler
            {
            public:
                POSIXIOSystem();
                virtual ~POSIXIOSystem();

                virtual bool servicesPath(const StringBuf& absoluteFilePath) override final;

                virtual FileHandlePtr openForReading(const StringBuf& absoluteFilePath) override final;
                virtual FileHandlePtr openForWriting(const StringBuf& absoluteFilePath, bool append) override final;
                virtual FileHandlePtr openForReadingAndWriting(const StringBuf& absoluteFilePath, bool resetContent = false) override final;

                virtual bool fileSize(const StringBuf& absoluteFilePath, uint64_t& outFileSize) override final;
                virtual bool fileTimeStamp(const StringBuf& absoluteFilePath, class TimeStamp& outTimeStamp) override final;
                virtual bool createPath(const StringBuf& absoluteFilePath) override final;
                virtual bool moveFile(const StringBuf& srcStringBuf, const StringBuf& destStringBuf) override final;
                virtual bool deleteFile(const StringBuf& absoluteFilePath) override final;
                virtual bool deleteDir(const StringBuf& absoluteDirPath) override final;
                virtual bool touchFile(const StringBuf& absoluteFilePath) override final;
                virtual bool fileExists(const StringBuf& absoluteFilePath) override final;
                virtual bool isFileReadOnly(const StringBuf& absoluteFilePath) override final;
                virtual bool readOnlyFlag(const StringBuf& absoluteFilePath, bool flag) override final;
                virtual void findFiles(const StringBuf& absoluteFilePath, const wchar_t* searchPattern, Array< StringBuf >& absoluteFiles, bool recurse) override final;
                virtual void findSubDirs(const StringBuf& absoluteFilePath, Array< UTF16StringVector >& outDirectoryNames) override final;
                virtual void findLocalFiles(const StringBuf& absoluteFilePath, const wchar_t* searchPattern, Array< UTF16StringVector >& outFileNames) override final;

                virtual void rootPaths(Array<BrowsableRoot>& outRoots) override final;
                virtual StringBuf systemPath(PathCategory category) override final;
                virtual DirectoryWatcherPtr createDirectoryWatcher(const StringBuf& path) override final;

                virtual void showFileExplorer(const StringBuf& path) override final;
                virtual bool showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<StringBuf>& outPaths, OpenSavePersistentData& persistentData) override final;
                virtual bool showFileSaveDialog(uint64_t nativeWindowHandle, const UTF16StringVector& currentFileName, const Array<FileFormat>& formats, StringBuf& outPath, OpenSavePersistentData& persistentData) override final;

            private:
                UniquePtr<POSIXAsyncReadDispatcher> m_asyncDispatcher;
                StringBuf m_rootPath;
            };

        } // prv
    } // io
} // base