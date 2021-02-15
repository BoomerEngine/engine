/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_io_glue.inl"

// Forward declarations that are very common

namespace base
{
    namespace io
    {

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

        class IIncludeHandler;

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

    } // io
} // base

#include "ioSystem.h"
