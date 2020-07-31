/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#pragma once

#include "absolutePath.h"

namespace base
{
    namespace io
    {
        //--

        // Allocate single block of memory from the memory pool
        // The IO memory is pre-allocated by the application so there's no risk of fragmentation-caused OOM or performance drop
        // If there's not enough free memory in the IO pool this function will wait (yield the fiber) until some memory is freed and it will retry
        // If a buffer is REALLY large (which usually means an error, than it's allocated directly from the system)
        extern BASE_IO_API CAN_YIELD Buffer AllocBlockAsync(uint32_t size);

        // Allocate atomically two blocks from the memory pool, this function will wait until both blocks can be allocated
        extern BASE_IO_API CAN_YIELD bool AllocBlocksAsync(uint32_t compressedSize, uint32_t decompressedSize, Buffer& outCompressedBuffer, Buffer& outDecompressedBuffer);

        //--

        /// mode for opening file for writing
        enum class FileWriteMode : uint8_t
        {
            DirectWrite, // directly open target file for writing, any writes will be permanent, potentially damaging the file
            DirectAppend, // directly open target file for appending stuff at the end
            StagedWrite, // write to a temporary file first, and if everything went well move it into target position, writing call be canceled by calling discardContent()
        };

        // Open physical file for reading
        extern BASE_IO_API ReadFileHandlePtr OpenForReading(AbsolutePathView absoluteFilePath);

        // open physical file for writing
        extern BASE_IO_API WriteFileHandlePtr OpenForWriting(AbsolutePathView absoluteFilePath, FileWriteMode mode = FileWriteMode::StagedWrite);

        // open physical file for async reading
        extern BASE_IO_API AsyncFileHandlePtr OpenForAsyncReading(AbsolutePathView absoluteFilePath);

        // create a READ ONLY memory mapped buffer view of a file, used by some asset loaders
        extern BASE_IO_API Buffer OpenMemoryMappedForReading(AbsolutePathView absoluteFilePath);

        //--

        //! Get file size, returns 0 if file does not exist (we are not interested in empty file either)
        extern BASE_IO_API bool FileSize(AbsolutePathView absoluteFilePath, uint64_t& outFileSize);

        //! Get file timestamp
        extern BASE_IO_API bool FileTimeStamp(AbsolutePathView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize = nullptr);

        //! Make sure all directories along the way exist
        extern BASE_IO_API bool CreatePath(AbsolutePathView absoluteFilePath);

        //! Copy file
        extern BASE_IO_API bool CopyFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath);

        //! Move file
        extern BASE_IO_API bool MoveFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath);

        //! Delete file from disk
        extern BASE_IO_API bool DeleteFile(AbsolutePathView absoluteFilePath);

        //! Delete folder from disk (note: directory must be empty)
        extern BASE_IO_API bool DeleteDir(AbsolutePathView absoluteDirPath);

        //! Check if file exists
        extern BASE_IO_API bool FileExists(AbsolutePathView absoluteFilePath);

        //! Check if file is read only
        extern BASE_IO_API bool IsFileReadOnly(AbsolutePathView absoluteFilePath);

        //! Change read only attribute on file
        extern BASE_IO_API bool ReadOnlyFlag(AbsolutePathView absoluteFilePath, bool flag);

        //! Touch file (updates the modification timestamp to current date without doing anything with the file)
        extern BASE_IO_API bool TouchFile(AbsolutePathView absoluteFilePath);

        //---

        //! Enumerate files in given directory, calls the handler for every file found, if handler returns "true" we assume the search is over
        extern BASE_IO_API bool FindFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(AbsolutePathView fullPath, StringView<wchar_t> fileName)>& enumFunc, bool recurse);

        //! Enumerate directories in given directory, calls the handler for every
        extern BASE_IO_API bool FindSubDirs(AbsolutePathView absoluteFilePath, const std::function<bool(StringView<wchar_t> name)>& enumFunc);

        //! Enumerate file in given directory, not recursive version, NOTE: slow as fuck
        extern BASE_IO_API bool FindLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(StringView<wchar_t> name)>& enumFunc);

        //---

        //! Enumerate files in given directory, NOTE: slow as fuck as strings are built
        extern BASE_IO_API void FindFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, Array< AbsolutePath >& outAbsoluteFiles, bool recurse);

        //! Enumerate directories in given directory, NOTE: slow as fuck as strings are built
        extern BASE_IO_API void FindSubDirs(AbsolutePathView absoluteFilePath, Array< UTF16StringBuf >& outDirectoryNames);

        //! Enumerate file in given directory, not recursive version, NOTE: slow as fuck as strings are built
        extern BASE_IO_API void FindLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, Array< UTF16StringBuf >& outFileNames);

        //----

          // type of location we want to retrieve from the system
        enum class PathCategory : uint8_t
        {
            // path to the executable file currently running
            ExecutableFile,

            // path to the bin/ directory we are running from
            ExecutableDir,

            // path to the temporary directory when temporary files may be stored
            // NOTE: temp directory may be totally purged between runs
            TempDir,

            // path to the user config directory - a directory where we can store user specific configuration files (like settings)
            UserConfigDir,

            // path to "My Documents" or /home/XXX/ or something similar
            UserDocumentsDir,

            //--

            MAX,
        };

        //! Get a path to some specific shit
        extern BASE_IO_API const AbsolutePath& SystemPath(PathCategory category);

        //---

        //! Create asynchronous directory watcher
        //! The watcher monitors directory for files being added/removed from it
        extern BASE_IO_API DirectoryWatcherPtr CreateDirectoryWatcher(AbsolutePathView path);

        //---

        /// persistent data for Open/Save file dialogs
        struct OpenSavePersistentData : public base::NoCopy
        {
            AbsolutePath directory;
            UTF16StringBuf userPattern;
            StringBuf filterExtension;
            UTF16StringBuf lastSaveFileName;
        };

        //! Show the given file in the file explorer
        extern BASE_IO_API void ShowFileExplorer(AbsolutePathView path);

        //! Show the system "Open File" dialog
        //! This pops up the native system window in which user can select a file(s) to be opened
        extern BASE_IO_API bool ShowFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<AbsolutePath>& outPaths, OpenSavePersistentData& persistentData);

        //! Show the system "Save File" dialog
        //! This pops up the native system window in which user can select a file(s) to be opened
        extern BASE_IO_API bool ShowFileSaveDialog(uint64_t nativeWindowHandle, const UTF16StringBuf& currentFileName, const Array<FileFormat>& formats, AbsolutePath& outPath, OpenSavePersistentData& persistentData);

        //---

        // load content of an absolute file on disk to a string buffer
        // returns true if content was loaded, false if there were errors (file does not exist, etc)
        // NOTE: both ANSI UTF-8 and UTF-16 files are supported, the UTF-16 files are automatically converted
        extern BASE_IO_API bool LoadFileToString(AbsolutePathView absoluteFilePath, StringBuf& outString);

        // load file content into a memory buffer, uses OS dependent implementation for maximum efficiency
        // NOTE: buffer is usually allocated outside of the normal memory pools
        extern BASE_IO_API Buffer LoadFileToBuffer(AbsolutePathView absoluteFilePath);

        //----

        // encoding of the file saved
        enum class StringEncoding : uint8_t
        {
            Ansi, // all chars > 255 are saved as ?
            UTF8, // native, no data conversion
            UTF16, // expands to 16-bits, larger values are written as ?
        };

        // save string (ANSI / UTF16) to file on disk
        // returns true if content was saved, false if there were errors (file could not be created, etc)
        extern BASE_IO_API bool SaveFileFromString(AbsolutePathView absoluteFilePath, StringView<char> str, StringEncoding encoding = StringEncoding::UTF8);

        // save block of memory to file on disk
        // returns true if content was saved, false if there were errors (file could not be created, etc)
        extern BASE_IO_API bool SaveFileFromBuffer(AbsolutePathView absoluteFilePath, const void* buffer, size_t size);

        // save block of memory to file on disk
        // returns true if content was saved, false if there were errors (file could not be created, etc)
        extern BASE_IO_API bool SaveFileFromBuffer(AbsolutePathView absoluteFilePath, const Buffer& buffer);

        //---

    } // io
} // base
