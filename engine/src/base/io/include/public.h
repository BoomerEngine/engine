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

        class AbsolutePath;

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

        class AbsolutePath;
        typedef StringView<wchar_t> AbsolutePathView;

    } // io
} // base

#include "ioSystem.h"
