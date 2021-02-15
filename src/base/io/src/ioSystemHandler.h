/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#pragma once

#include "base/containers/include/array.h"
#include "ioSystem.h"

namespace base
{
    namespace io
    {
        namespace prv
        {
            /// low-level IO system handler
            class BASE_IO_API ISystemHandler
            {
            public:
                ISystemHandler();
                virtual ~ISystemHandler();

                //----

                // open physical file for reading
                virtual ReadFileHandlePtr openForReading(StringView absoluteFilePath) = 0;

                // open physical file for writing
                virtual WriteFileHandlePtr openForWriting(StringView absoluteFilePath, FileWriteMode mode = FileWriteMode::StagedWrite) = 0;

                // open physical file for async reading
                virtual AsyncFileHandlePtr openForAsyncReading(StringView absoluteFilePath) = 0;

                // load file content into a buffer
                virtual Buffer loadIntoMemoryForReading(StringView absoluteFilePath) = 0;

                // open a read only memory mapped access to file
                virtual Buffer openMemoryMappedForReading(StringView absoluteFilePath) = 0;

                //! Get file size, returns 0 if file does not exist (we are not interested in empty file either)
                virtual bool fileSize(StringView absoluteFilePath, uint64_t& outFileSize) = 0;

                //! Get file timestamp
                virtual bool fileTimeStamp(StringView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize) = 0;

                //! Make sure all directories along the way exist
                virtual bool createPath(StringView absoluteFilePath) = 0;

                //! Move file
                virtual bool moveFile(StringView srcAbsolutePath, StringView destAbsolutePath) = 0;

                //! Copy file
                virtual bool copyFile(StringView srcAbsolutePath, StringView destAbsolutePath) = 0;

                //! Delete file from disk
                virtual bool deleteFile(StringView absoluteFilePath) = 0;

                //! Delete folder from disk
                virtual bool deleteDir(StringView absoluteDirPath) = 0;

                //! Update modification date on the file
                virtual bool touchFile(StringView absoluteFilePath) = 0;

                //! Check if file exists
                virtual bool fileExists(StringView absoluteFilePath) = 0;

                //! Check if file is read only
                virtual bool isFileReadOnly(StringView absoluteFilePath) = 0;

                //! Change read only attribute on file
                virtual bool readOnlyFlag(StringView absoluteFilePath, bool flag) = 0;

                //! Enumerate files in given directory, NOTE: slow as fuck
                virtual bool findFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView fullPath, StringView fileName)>& enumFunc, bool recurse) = 0;

                //! Enumerate directories in given directory, NOTE: slow as fuck
                virtual bool findSubDirs(StringView absoluteFilePath, const std::function<bool(StringView name)>& enumFunc) = 0;

                //! Enumerate file in given directory, not recursive version, NOTE: slow as fuck
                virtual bool findLocalFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView name)>& enumFunc) = 0;

                //! Get a path to some specific shit
                virtual void systemPath(PathCategory category, IFormatStream& f) = 0;

                //! Create asynchronous directory watcher
                virtual DirectoryWatcherPtr createDirectoryWatcher(StringView path) = 0;

                //! Show the given file in the file explorer
                virtual void showFileExplorer(StringView path) = 0;

                //! Show the system "Open File" dialog
                //! This pops up the native system window in which user can select a file(s) to be opened
                virtual bool showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<StringBuf>& outPaths, OpenSavePersistentData& persistentData) = 0;

                //! Show the system "Save File" dialog
                //! This pops up the native system window in which user can select a file(s) to be opened
                virtual bool showFileSaveDialog(uint64_t nativeWindowHandle, const StringBuf& currentFileName, const Array<FileFormat>& formats, StringBuf& outPath, OpenSavePersistentData& persistentData) = 0;
            };

        } // prv
    } // io
} // base
