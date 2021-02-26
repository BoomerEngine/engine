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

BEGIN_BOOMER_NAMESPACE_EX(io)

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

        virtual ReadFileHandlePtr openForReading(StringView absoluteFilePath) override final;
        virtual WriteFileHandlePtr openForWriting(StringView absoluteFilePath, FileWriteMode mode = FileWriteMode::StagedWrite) override final;
        virtual AsyncFileHandlePtr openForAsyncReading(StringView absoluteFilePath) override final;

        virtual Buffer loadIntoMemoryForReading(StringView absoluteFilePath) override final;
        virtual Buffer openMemoryMappedForReading(StringView absoluteFilePath) override final;

        virtual bool fileSize(StringView absoluteFilePath, uint64_t& outFileSize) override final;
        virtual bool fileTimeStamp(StringView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize) override final;
        virtual bool createPath(StringView absoluteFilePath) override final;
        virtual bool moveFile(StringView srcAbsolutePath, StringView destAbsolutePath) override final;
        virtual bool copyFile(StringView srcAbsolutePath, StringView destAbsolutePath) override final;
        virtual bool deleteFile(StringView absoluteFilePath) override final;
		virtual bool deleteDir(StringView absoluteDirPath) override final;
        virtual bool touchFile(StringView absoluteFilePath) override final;
        virtual bool fileExists(StringView absoluteFilePath) override final;
        virtual bool isFileReadOnly(StringView absoluteFilePath) override final;
        virtual bool readOnlyFlag(StringView absoluteFilePath, bool flag) override final;
        virtual bool findFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView fullPath, StringView fileName)>& enumFunc, bool recurse) override final;
        virtual bool findSubDirs(StringView absoluteFilePath, const std::function<bool(StringView name)>& enumFunc) override final;
        virtual bool findLocalFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView name)>& enumFunc) override final;

        virtual void systemPath(PathCategory category, IFormatStream& f) override final;
        virtual DirectoryWatcherPtr createDirectoryWatcher(StringView path) override final;

        virtual void showFileExplorer(StringView path) override final;
        virtual bool showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, Array<StringBuf>& outPaths, OpenSavePersistentData& persistentData) override final;
        virtual bool showFileSaveDialog(uint64_t nativeWindowHandle, const StringBuf& currentFileName, const Array<FileFormat>& formats, StringBuf& outPath, OpenSavePersistentData& persistentData) override final;

        //--

        WriteFileHandlePtr openForWriting(const wchar_t* absoluteFilePath, bool append);

    private:
        WinAsyncReadDispatcher* m_asyncDispatcher;

        virtual void deinit() override;

        bool findFilesInternal(TempPathStringBuffer& dirPath, TempPathStringBufferAnsi& dirPathUTF, StringView searchPattern, const std::function<bool(StringView fullPath, StringView fileName)>& enumFunc, bool recurse);
    };

} // prv

END_BOOMER_NAMESPACE_EX(io)
