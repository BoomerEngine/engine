/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\watcher #]
***/

#include "build.h"
#include "ioDirectoryWatcher.h"

namespace base
{
    namespace io
    {

        IDirectoryWatcherListener::~IDirectoryWatcherListener()
        {}

        IDirectoryWatcher::~IDirectoryWatcher()
        {}

        IDirectoryWatcher::IDirectoryWatcher()
        {}

    } // io
} // base