/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#pragma once

#include "absolutePath.h"
#include "ioMemoryPool.h"

#include "base/containers/include/array.h"
#include "base/containers/include/inplaceArray.h"
#include "base/system/include/commandline.h"

namespace base
{
    namespace io
    {
        class AbsolutePath;
        typedef StringView<wchar_t> AbsolutePathView;

        namespace prv
        {
            class ISystemHandler;
        } // prv

        /// persistent data for Open/Save file dialogs
        struct OpenSavePersistentData : public base::NoCopy
        {
            AbsolutePath directory;
            UTF16StringBuf userPattern;
            StringBuf filterExtension;
        };

        /// abstract IO system
        class BASE_IO_API System : public ISingleton
        {
            DECLARE_SINGLETON(System);

        public:
            //----

            // get internal memory pool for IO operations
            // NOTE: especially short-lived async reading should use memory from this pool
            INLINE MemoryPool& memoryPool() { return *m_memoryPool; }

            //----

            // open physical file for reading
            FileHandlePtr openForReading(AbsolutePathView absoluteFilePath);

            // open physical file for writing
            FileHandlePtr openForWriting(AbsolutePathView absoluteFilePath, bool append = false);

            // open physical file for reading and writing at the same time (cache file, package, etc)
            FileHandlePtr openForReadingAndWriting(AbsolutePathView absoluteFilePath, bool resetContent = false);

            //--

            // create a READ ONLY memory mapped buffer view of a file
            Buffer openMemoryMappedForReading(AbsolutePathView absoluteFilePath);

            // load file content into a buffer
            Buffer loadIntoMemoryForReading(AbsolutePathView absoluteFilePath);

            //----

            //! Get file size, returns 0 if file does not exist (we are not interested in empty file either)
            bool fileSize(AbsolutePathView absoluteFilePath, uint64_t& outFileSize);

            //! Get file timestamp
            bool fileTimeStamp(AbsolutePathView absoluteFilePath, class TimeStamp& outTimeStamp, uint64_t* outFileSize = nullptr);

            //! Make sure all directories along the way exist
            bool createPath(AbsolutePathView absoluteFilePath);

            //! Copy file
            bool copyFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath);

            //! Move file
            bool moveFile(AbsolutePathView srcAbsolutePath, AbsolutePathView destAbsolutePath);

            //! Delete file from disk
            bool deleteFile(AbsolutePathView absoluteFilePath);

            //! Delete folder from disk (note: directory must be empty)
            bool deleteDir(AbsolutePathView absoluteDirPath);

            //! Check if file exists
            bool fileExists(AbsolutePathView absoluteFilePath);

            //! Check if file is read only
            bool isFileReadOnly(AbsolutePathView absoluteFilePath);

            //! Change read only attribute on file
            bool readOnlyFlag(AbsolutePathView absoluteFilePath, bool flag);

            //! Touch file (updates the modification timestamp to current date without doing anything with the file)
            bool touchFile(AbsolutePathView absoluteFilePath);

            //! Enumerate files in given directory, calls the handler for every file found, if handler returns "true" we assume the search is over
            bool findFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(AbsolutePathView fullPath, StringView<wchar_t> fileName)>& enumFunc, bool recurse);

            //! Enumerate directories in given directory, calls the handler for every
            bool findSubDirs(AbsolutePathView absoluteFilePath, const std::function<bool(StringView<wchar_t> name)>& enumFunc);

            //! Enumerate file in given directory, not recursive version, NOTE: slow as fuck
            bool findLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, const std::function<bool(StringView<wchar_t> name)>& enumFunc);

            //--

            //! Enumerate files in given directory, NOTE: slow as fuck as strings are built
            void findFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, Array< AbsolutePath >& outAbsoluteFiles, bool recurse);

            //! Enumerate directories in given directory, NOTE: slow as fuck as strings are built
            void findSubDirs(AbsolutePathView absoluteFilePath, Array< UTF16StringBuf >& outDirectoryNames);

            //! Enumerate file in given directory, not recursive version, NOTE: slow as fuck as strings are built
            void findLocalFiles(AbsolutePathView absoluteFilePath, StringView<wchar_t> searchPattern, Array< UTF16StringBuf >& outFileNames);

            //---
            //! Get a path to some specific shit
            const AbsolutePath& systemPath(PathCategory category);

            //---

            //! Create asynchronous directory watcher
            //! The watcher monitors directory for files being added/removed from it
            DirectoryWatcherPtr createDirectoryWatcher(AbsolutePathView path);

            //---

            //! Show the given file in the file explorer
            void showFileExplorer(AbsolutePathView path);

            //! Show the system "Open File" dialog
            //! This pops up the native system window in which user can select a file(s) to be opened
            bool showFileOpenDialog(uint64_t nativeWindowHandle, bool allowMultiple, const Array<FileFormat>& formats, base::Array<AbsolutePath>& outPaths, OpenSavePersistentData& persistentData);

            //! Show the system "Save File" dialog
            //! This pops up the native system window in which user can select a file(s) to be opened
            bool showFileSaveDialog(uint64_t nativeWindowHandle, const UTF16StringBuf& currentFileName, const Array<FileFormat>& formats, AbsolutePath& outPath, OpenSavePersistentData& persistentData);

        private:
            System();

            // cached system paths
            AbsolutePath m_paths[(uint8_t)PathCategory::MAX];

            // memory pool for IO operations
            UniquePtr<MemoryPool> m_memoryPool;

            // actual file system handler
            prv::ISystemHandler* m_handler;

            //--

            virtual void deinit() override;
        };

    } // io

} // base

/// the IO system is global
using IO = base::io::System;