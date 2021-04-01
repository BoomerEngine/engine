/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_io_glue.inl"

// Forward declarations that are very common

BEGIN_BOOMER_NAMESPACE()

class IReadFileHandle;
typedef RefPtr<IReadFileHandle> ReadFileHandlePtr;

class IWriteFileHandle;
typedef RefPtr<IWriteFileHandle> WriteFileHandlePtr;

class IAsyncFileHandle;
typedef RefPtr<IAsyncFileHandle> AsyncFileHandlePtr;

class IDirectoryWatcher;
class IDirectoryWatcherListener;
typedef RefPtr<IDirectoryWatcher> DirectoryWatcherPtr;

class DirectoryMonitor;

class MemoryPool;

class TimeStamp;

class FileFormat;

class CRCCache;

static const char WINDOWS_PATH_SEPARATOR = '\\';
static const char UNIX_PATH_SEPARATOR = '/';

#if defined(PLATFORM_WINDOWS)
static const char SYSTEM_PATH_SEPARATOR = WINDOWS_PATH_SEPARATOR;
static const char WRONG_SYSTEM_PATH_SEPARATOR = UNIX_PATH_SEPARATOR;
#else
static const char SYSTEM_PATH_SEPARATOR = UNIX_PATH_SEPARATOR;
static const char WRONG_SYSTEM_PATH_SEPARATOR = WINDOWS_PATH_SEPARATOR;
#endif

//--

// Allocate single block of memory from the memory pool
// The IO memory is pre-allocated by the application so there's no risk of fragmentation-caused OOM or performance drop
// If there's not enough free memory in the IO pool this function will wait (yield the fiber) until some memory is freed and it will retry
// If a buffer is REALLY large (which usually means an error, than it's allocated directly from the system)
extern CORE_IO_API CAN_YIELD Buffer AllocBlockAsync(uint32_t size);

// Allocate atomically two blocks from the memory pool, this function will wait until both blocks can be allocated
extern CORE_IO_API CAN_YIELD bool AllocBlocksAsync(uint32_t compressedSize, uint32_t decompressedSize, Buffer& outCompressedBuffer, Buffer& outDecompressedBuffer);

//--

/// mode for opening file for writing
enum class FileWriteMode : uint8_t
{
    DirectWrite, // directly open target file for writing, any writes will be permanent, potentially damaging the file
    DirectAppend, // directly open target file for appending stuff at the end
    StagedWrite, // write to a temporary file first, and if everything went well move it into target position, writing call be canceled by calling discardContent()
};

// Open physical file for reading
extern CORE_IO_API ReadFileHandlePtr OpenForReading(StringView absoluteFilePath, TimeStamp* outTimestamp = nullptr);

// open physical file for writing
extern CORE_IO_API WriteFileHandlePtr OpenForWriting(StringView absoluteFilePath, FileWriteMode mode = FileWriteMode::StagedWrite);

// open physical file for async reading
extern CORE_IO_API AsyncFileHandlePtr OpenForAsyncReading(StringView absoluteFilePath, TimeStamp* outTimestamp = nullptr);

// create a READ ONLY memory mapped buffer view of a file, used by some asset loaders
extern CORE_IO_API Buffer OpenMemoryMappedForReading(StringView absoluteFilePath);

//--

//! Get file size, returns 0 if file does not exist (we are not interested in empty file either)
extern CORE_IO_API bool FileSize(StringView absoluteFilePath, uint64_t& outFileSize);

//! Get file timestamp
extern CORE_IO_API bool FileTimeStamp(StringView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize = nullptr);

//! Make sure all directories along the way exist
extern CORE_IO_API bool CreatePath(StringView absoluteFilePath);

//! Copy file
extern CORE_IO_API bool CopyFile(StringView srcAbsolutePath, StringView destAbsolutePath);

//! Move file
extern CORE_IO_API bool MoveFile(StringView srcAbsolutePath, StringView destAbsolutePath);

//! Delete file from disk
extern CORE_IO_API bool DeleteFile(StringView absoluteFilePath);

//! Delete folder from disk (note: directory must be empty)
extern CORE_IO_API bool DeleteDir(StringView absoluteDirPath);

//! Check if file exists
extern CORE_IO_API bool FileExists(StringView absoluteFilePath);

//! Check if file is read only
extern CORE_IO_API bool IsFileReadOnly(StringView absoluteFilePath);

//! Change read only attribute on file
extern CORE_IO_API bool ReadOnlyFlag(StringView absoluteFilePath, bool flag);

//! Touch file (updates the modification timestamp to current date without doing anything with the file)
extern CORE_IO_API bool TouchFile(StringView absoluteFilePath);

//---

//! Enumerate files in given directory, calls the handler for every file found, if handler returns "true" we assume the search is over
extern CORE_IO_API bool FindFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView fullPath, StringView fileName)>& enumFunc, bool recurse);

//! Enumerate directories in given directory, calls the handler for every
extern CORE_IO_API bool FindSubDirs(StringView absoluteFilePath, const std::function<bool(StringView name)>& enumFunc);

//! Enumerate file in given directory, not recursive version, NOTE: slow as fuck
extern CORE_IO_API bool FindLocalFiles(StringView absoluteFilePath, StringView searchPattern, const std::function<bool(StringView name)>& enumFunc);

//---

//! Enumerate files in given directory, NOTE: slow as fuck as strings are built
extern CORE_IO_API void FindFiles(StringView absoluteFilePath, StringView searchPattern, Array<StringBuf>& outAbsoluteFiles, bool recurse);

//! Enumerate directories in given directory, NOTE: slow as fuck as strings are built
extern CORE_IO_API void FindSubDirs(StringView absoluteFilePath, Array<StringBuf>& outDirectoryNames);

//! Enumerate file in given directory, not recursive version, NOTE: slow as fuck as strings are built
extern CORE_IO_API void FindLocalFiles(StringView absoluteFilePath, StringView searchPattern, Array<StringBuf>& outFileNames);

//----

// type of location we want to retrieve from the system
enum class PathCategory : uint8_t
{
    // path to the executable file currently running
    ExecutableFile,

    // path to the bin/ directory we are running from
    ExecutableDir,

    // shared binary directory (shared/ or bin/shared), contains all not-linked utilities
    SharedDir,

    // engine directory (contains engine's src, data and config)
    EngineDir,

    // path to the system wide temporary directory when temporary files may be stored
    // NOTE: temp directory may be totally purged between runs
    SystemTempDir,

    // project local temp directory that has higher tendency to stay around
    LocalTempDir,

    // path to the user config directory - a directory where we can store user specific configuration files (like settings)
    UserConfigDir,

    // path to "My Documents" or /home/XXX/ or something similar
    UserDocumentsDir,

    //--

    MAX,
};

//! Get a path to some specific shit
extern CORE_IO_API const StringBuf& SystemPath(PathCategory category);

//---

//! Create asynchronous directory watcher
//! The watcher monitors directory for files being added/removed from it
extern CORE_IO_API DirectoryWatcherPtr CreateDirectoryWatcher(StringView path);

//---

/// persistent data for Open/Save file dialogs
struct OpenSavePersistentData
{
RTTI_DECLARE_POOL(POOL_IO)

public:
    StringBuf directory;
    StringBuf userPattern;
    StringBuf filterExtension;
    StringBuf lastSaveFileName;
};

//! Show the given file in the file explorer
extern CORE_IO_API void ShowFileExplorer(StringView path);

//! Show the system "Open File" dialog
//! This pops up the native system window in which user can select a file(s) to be opened
extern CORE_IO_API bool ShowFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, Array<StringBuf>& outPaths, OpenSavePersistentData& persistentData);

//! Show the system "Save File" dialog
//! This pops up the native system window in which user can select a file(s) to be opened
extern CORE_IO_API bool ShowFileSaveDialog(uint64_t nativeWindowHandle, const StringBuf& currentFileName, const Array<FileFormat>& formats, StringBuf& outPath, OpenSavePersistentData& persistentData);

//---

// load content of an absolute file on disk to a string buffer
// returns true if content was loaded, false if there were errors (file does not exist, etc)
// NOTE: both ANSI UTF-8 and UTF-16 files are supported, the UTF-16 files are automatically converted
extern CORE_IO_API bool LoadFileToString(StringView absoluteFilePath, StringBuf& outString);

// load file content into a memory buffer, uses OS dependent implementation for maximum efficiency
// NOTE: buffer is usually allocated outside of the normal memory pools
extern CORE_IO_API Buffer LoadFileToBuffer(StringView absoluteFilePath);

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
extern CORE_IO_API bool SaveFileFromString(StringView absoluteFilePath, StringView str, StringEncoding encoding = StringEncoding::UTF8);

// save block of memory to file on disk
// returns true if content was saved, false if there were errors (file could not be created, etc)
extern CORE_IO_API bool SaveFileFromBuffer(StringView absoluteFilePath, const void* buffer, size_t size);

// save block of memory to file on disk
// returns true if content was saved, false if there were errors (file could not be created, etc)
extern CORE_IO_API bool SaveFileFromBuffer(StringView absoluteFilePath, const Buffer& buffer);

//---

END_BOOMER_NAMESPACE()
