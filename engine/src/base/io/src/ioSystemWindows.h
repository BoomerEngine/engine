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
            class TempPathStringBufferAnsi;

            class WinIOSystem : public ISystemHandler, public ISingleton
            {
                DECLARE_SINGLETON(WinIOSystem);

            public:
                WinIOSystem();

                virtual ReadFileHandlePtr openForReading(StringView<char> absoluteFilePath) override final;
                virtual WriteFileHandlePtr openForWriting(StringView<char> absoluteFilePath, FileWriteMode mode = FileWriteMode::StagedWrite) override final;
                virtual AsyncFileHandlePtr openForAsyncReading(StringView<char> absoluteFilePath) override final;

                virtual Buffer loadIntoMemoryForReading(StringView<char> absoluteFilePath) override final;
                virtual Buffer openMemoryMappedForReading(StringView<char> absoluteFilePath) override final;

                virtual bool fileSize(StringView<char> absoluteFilePath, uint64_t& outFileSize) override final;
                virtual bool fileTimeStamp(StringView<char> absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize) override final;
                virtual bool createPath(StringView<char> absoluteFilePath) override final;
                virtual bool moveFile(StringView<char> srcAbsolutePath, StringView<char> destAbsolutePath) override final;
                virtual bool copyFile(StringView<char> srcAbsolutePath, StringView<char> destAbsolutePath) override final;
                virtual bool deleteFile(StringView<char> absoluteFilePath) override final;
				virtual bool deleteDir(StringView<char> absoluteDirPath) override final;
                virtual bool touchFile(StringView<char> absoluteFilePath) override final;
                virtual bool fileExists(StringView<char> absoluteFilePath) override final;
                virtual bool isFileReadOnly(StringView<char> absoluteFilePath) override final;
                virtual bool readOnlyFlag(StringView<char> absoluteFilePath, bool flag) override final;
                virtual bool findFiles(StringView<char> absoluteFilePath, StringView<char> searchPattern, const std::function<bool(StringView<char> fullPath, StringView<char> fileName)>& enumFunc, bool recurse) override final;
                virtual bool findSubDirs(StringView<char> absoluteFilePath, const std::function<bool(StringView<char> name)>& enumFunc) override final;
                virtual bool findLocalFiles(StringView<char> absoluteFilePath, StringView<char> searchPattern, const std::function<bool(StringView<char> name)>& enumFunc) override final;

                virtual void systemPath(PathCategory category, IFormatStream& f) override final;
                virtual DirectoryWatcherPtr createDirectoryWatcher(StringView<char> path) override final;

                virtual void showFileExplorer(StringView<char> path) override final;
                virtual bool showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<StringBuf>& outPaths, OpenSavePersistentData& persistentData) override final;
                virtual bool showFileSaveDialog(uint64_t nativeWindowHandle, const StringBuf& currentFileName, const Array<FileFormat>& formats, StringBuf& outPath, OpenSavePersistentData& persistentData) override final;

                //--

                WriteFileHandlePtr openForWriting(const wchar_t* absoluteFilePath, bool append);

            private:
                WinAsyncReadDispatcher* m_asyncDispatcher;

                virtual void deinit() override;

                bool findFilesInternal(TempPathStringBuffer& dirPath, TempPathStringBufferAnsi& dirPathUTF, StringView<char> searchPattern, const std::function<bool(StringView<char> fullPath, StringView<char> fileName)>& enumFunc, bool recurse);
            };

        } // prv
    } // io
} // base