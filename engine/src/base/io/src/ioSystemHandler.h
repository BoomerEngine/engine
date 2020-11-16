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
                virtual ReadFileHandlePtr openForReading(StringView<char> absoluteFilePath) = 0;

                // open physical file for writing
                virtual WriteFileHandlePtr openForWriting(StringView<char> absoluteFilePath, FileWriteMode mode = FileWriteMode::StagedWrite) = 0;

                // open physical file for async reading
                virtual AsyncFileHandlePtr openForAsyncReading(StringView<char> absoluteFilePath) = 0;

                // load file content into a buffer
                virtual Buffer loadIntoMemoryForReading(StringView<char> absoluteFilePath) = 0;

                // open a read only memory mapped access to file
                virtual Buffer openMemoryMappedForReading(StringView<char> absoluteFilePath) = 0;

                //! Get file size, returns 0 if file does not exist (we are not interested in empty file either)
                virtual bool fileSize(StringView<char> absoluteFilePath, uint64_t& outFileSize) = 0;

                //! Get file timestamp
                virtual bool fileTimeStamp(StringView<char> absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize) = 0;

                //! Make sure all directories along the way exist
                virtual bool createPath(StringView<char> absoluteFilePath) = 0;

                //! Move file
                virtual bool moveFile(StringView<char> srcAbsolutePath, StringView<char> destAbsolutePath) = 0;

                //! Copy file
                virtual bool copyFile(StringView<char> srcAbsolutePath, StringView<char> destAbsolutePath) = 0;

                //! Delete file from disk
                virtual bool deleteFile(StringView<char> absoluteFilePath) = 0;

                //! Delete folder from disk
                virtual bool deleteDir(StringView<char> absoluteDirPath) = 0;

                //! Update modification date on the file
                virtual bool touchFile(StringView<char> absoluteFilePath) = 0;

                //! Check if file exists
                virtual bool fileExists(StringView<char> absoluteFilePath) = 0;

                //! Check if file is read only
                virtual bool isFileReadOnly(StringView<char> absoluteFilePath) = 0;

                //! Change read only attribute on file
                virtual bool readOnlyFlag(StringView<char> absoluteFilePath, bool flag) = 0;

                //! Enumerate files in given directory, NOTE: slow as fuck
                virtual bool findFiles(StringView<char> absoluteFilePath, StringView<char> searchPattern, const std::function<bool(StringView<char> fullPath, StringView<char> fileName)>& enumFunc, bool recurse) = 0;

                //! Enumerate directories in given directory, NOTE: slow as fuck
                virtual bool findSubDirs(StringView<char> absoluteFilePath, const std::function<bool(StringView<char> name)>& enumFunc) = 0;

                //! Enumerate file in given directory, not recursive version, NOTE: slow as fuck
                virtual bool findLocalFiles(StringView<char> absoluteFilePath, StringView<char> searchPattern, const std::function<bool(StringView<char> name)>& enumFunc) = 0;

                //! Get a path to some specific shit
                virtual void systemPath(PathCategory category, IFormatStream& f) = 0;

                //! Create asynchronous directory watcher
                virtual DirectoryWatcherPtr createDirectoryWatcher(StringView<char> path) = 0;

                //! Show the given file in the file explorer
                virtual void showFileExplorer(StringView<char> path) = 0;

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
