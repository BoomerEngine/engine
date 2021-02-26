/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\watcher\posix #]
***/

#pragma once

#include "ioDirectoryWatcher.h"

#include "core/system/include/spinLock.h"
#include "core/containers/include/array.h"
#include "core/containers/include/hashMap.h"
#include "core/system/include/mutex.h"
#include "core/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE_EX(io)

namespace prv
{

    /// POSIX implementation of the watcher
    class POSIXDirectoryWatcher : public IDirectoryWatcher
    {
    public:
        POSIXDirectoryWatcher(StringView rootPath);
        virtual ~POSIXDirectoryWatcher();

        //! attach listener
        virtual void attachListener(IDirectoryWatcherListener* listener) override;
        virtual void dettachListener(IDirectoryWatcherListener* listener) override;

    private:
        static const uint32_t BUF_LEN = 1024 * 64;

        int m_masterHandle;

        Mutex m_listenersLock;
        Array<IDirectoryWatcherListener*> m_listeners;

        SpinLock m_mapLock;
        HashMap<int, StringBuf> m_handleToPath;
        HashMap<uint64_t, int> m_pathToHandle;

        Thread m_localThread;

        Array<DirectoryWatcherEvent> m_tempEvents;
        Array<StringBuf> m_tempAddedDirectories;
        Array<StringBuf> m_tempRemovedDirectories;

        Array<StringBuf> m_filesCreatedButNotYetClosed;
        Array<StringBuf> m_filesModifiedButNotYetClosed;

        uint8_t m_buffer[BUF_LEN];

        void monitorPath(StringView path);
        void unmonitorPath(StringView path);

        void watch();
    };

} // prv

END_BOOMER_NAMESPACE_EX(io)
