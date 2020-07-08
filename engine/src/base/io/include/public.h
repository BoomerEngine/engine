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

            //--

            MAX,
        };

        class AbsolutePath;

        class IFileHandle;
        typedef RefPtr<IFileHandle> FileHandlePtr;

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
