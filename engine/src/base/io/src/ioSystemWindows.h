/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#pragma once

#include "ioSystemHandler.h"
#include "ioFileHandle.h"

namespace base
{
    namespace io
    {
        namespace prv
        {

            class WinAsyncReadDispatcher;
            class TempPathStringBuffer;

            class WinIOSystem : public ISystemHandler, public ISingleton
            {
                DECLARE_SINGLETON(WinIOSystem);

            public:
                WinIOSystem();

                virtual ReadFileHandlePtr openForReading(AbsolutePathView absoluteFilePath) override final;
                virtual WriteFileHandlePtr openForWriting(AbsolutePathView absoluteFilePath, FileWriteMode mode = FileWriteMode::StagedWrite) override final;
                virtual AsyncFileHandlePtr openForAsyncReading(AbsolutePathView absoluteFilePath) override final;

                virtual Buffer loadIntoMemoryForReading(AbsolutePathView absoluteFilePath) override final;
                virtual Buffer openMemoryMappedForReading(AbsolutePathView absoluteFilePath) override final;

                virtual bool fileSize(AbsolutePathView absoluteFilePath, uint64_t& outFileSize) override final;
                virtual bool fileTimeStamp(AbsolutePathView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize) override final;
                virtual bool createPath(AbsolutePathView absoluteFilePath) override final;
                virtual bool moveFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath) override final;
                virtual bool copyFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath) override final;
                virtual bool deleteFile(AbsolutePathView absoluteFilePath) override final;
				virtual bool deleteDir(AbsolutePathView absoluteDirPath) override final;
                virtual bool touchFile(AbsolutePathView absoluteFilePath) override final;
                virtual bool fileExists(AbsolutePathView absoluteFilePath) override final;
                virtual bool isFileReadOnly(AbsolutePathView absoluteFilePath) override final;
                virtual bool readOnlyFlag(AbsolutePathView absoluteFilePath, bool flag) override final;
                virtual bool findFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(AbsolutePathView fullPath, StringView<wchar_t> fileName)>& enumFunc, bool recurse) override final;
                virtual bool findSubDirs(AbsolutePathView absoluteFilePath, const std::function<bool(StringView<wchar_t> name)>& enumFunc) override final;
                virtual bool findLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(StringView<wchar_t> name)>& enumFunc) override final;

                virtual AbsolutePath systemPath(PathCategory category) override final;
                virtual DirectoryWatcherPtr createDirectoryWatcher(AbsolutePathView path) override final;

                virtual void showFileExplorer(AbsolutePathView path) override final;
                virtual bool showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<AbsolutePath>& outPaths, OpenSavePersistentData& persistentData) override final;
                virtual bool showFileSaveDialog(uint64_t nativeWindowHandle, const UTF16StringBuf& currentFileName, const Array<FileFormat>& formats, AbsolutePath& outPath, OpenSavePersistentData& persistentData) override final;

            private:
                WinAsyncReadDispatcher* m_asyncDispatcher;

                virtual void deinit() override;

                bool findFilesInternal(TempPathStringBuffer& dirPath, StringView<wchar_t> searchPattern, const std::function<bool(AbsolutePathView fullPath, StringView<wchar_t> fileName)>& enumFunc, bool recurse);
            };

        } // prv
    } // io
} // base