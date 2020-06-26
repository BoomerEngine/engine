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

                virtual bool servicesPath(const AbsolutePath& absoluteFilePath) override final;

                virtual FileHandlePtr openForReading(const AbsolutePath& absoluteFilePath) override final;
                virtual FileHandlePtr openForWriting(const AbsolutePath& absoluteFilePath, bool append) override final;
                virtual FileHandlePtr openForReadingAndWriting(const AbsolutePath& absoluteFilePath, bool resetContent = false) override final;

                virtual bool fileSize(const AbsolutePath& absoluteFilePath, uint64_t& outFileSize) override final;
                virtual bool fileTimeStamp(const AbsolutePath& absoluteFilePath, class TimeStamp& outTimeStamp) override final;
                virtual bool createPath(const AbsolutePath& absoluteFilePath) override final;
                virtual bool moveFile(const AbsolutePath& srcAbsolutePath, const AbsolutePath& destAbsolutePath) override final;
                virtual bool deleteFile(const AbsolutePath& absoluteFilePath) override final;
                virtual bool deleteDir(const AbsolutePath& absoluteDirPath) override final;
                virtual bool touchFile(const AbsolutePath& absoluteFilePath) override final;
                virtual bool fileExists(const AbsolutePath& absoluteFilePath) override final;
                virtual bool isFileReadOnly(const AbsolutePath& absoluteFilePath) override final;
                virtual bool readOnlyFlag(const AbsolutePath& absoluteFilePath, bool flag) override final;
                virtual void findFiles(const AbsolutePath& absoluteFilePath, const wchar_t* searchPattern, Array< AbsolutePath >& absoluteFiles, bool recurse) override final;
                virtual void findSubDirs(const AbsolutePath& absoluteFilePath, Array< UTF16StringBuf >& outDirectoryNames) override final;
                virtual void findLocalFiles(const AbsolutePath& absoluteFilePath, const wchar_t* searchPattern, Array< UTF16StringBuf >& outFileNames) override final;

                virtual void rootPaths(Array<BrowsableRoot>& outRoots) override final;
                virtual AbsolutePath systemPath(PathCategory category) override final;
                virtual DirectoryWatcherPtr createDirectoryWatcher(const AbsolutePath& path) override final;

                virtual void showFileExplorer(const AbsolutePath& path) override final;
                virtual bool showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<AbsolutePath>& outPaths, OpenSavePersistentData& persistentData) override final;
                virtual bool showFileSaveDialog(uint64_t nativeWindowHandle, const UTF16StringBuf& currentFileName, const Array<FileFormat>& formats, AbsolutePath& outPath, OpenSavePersistentData& persistentData) override final;

            private:
                UniquePtr<POSIXAsyncReadDispatcher> m_asyncDispatcher;
                AbsolutePath m_rootPath;
            };

        } // prv
    } // io
} // base