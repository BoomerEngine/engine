/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\watcher\winapi #]
***/

#pragma once

#include <Windows.h>

#include "ioDirectoryWatcher.h"
#include "core/system/include/thread.h"
#include "core/system/include/timing.h"

BEGIN_BOOMER_NAMESPACE_EX(io)

namespace prv
{

    /// POSIX implementation of the watcher
    class WinDirectoryWatcher : public IDirectoryWatcher
    {
    public:
        WinDirectoryWatcher(Array<wchar_t> rootPath);
        virtual ~WinDirectoryWatcher();

        //! attach listener
        virtual void attachListener(IDirectoryWatcherListener* listener) override;
        virtual void dettachListener(IDirectoryWatcherListener* listener) override;

    private:
        Array<wchar_t> m_watchedPath;

        Mutex m_listenersLock;
        Array<IDirectoryWatcherListener*> m_listeners;
        bool m_listenersIterated = false;

        Thread m_localThread;
        Thread m_dispatchThread;

        OVERLAPPED m_waitCondition;
        HANDLE m_directoryHandle;

        std::atomic<bool> m_requestExit = false;

        Array<DirectoryWatcherEvent> m_pendingEvents;
        SpinLock m_pendingEventsLock;

        struct PendingModification
        {
            StringBuf path;
            NativeTimePoint time;
            NativeTimePoint expires;
        };

        Array<PendingModification> m_pendingModifications;
        SpinLock m_pendingModificationsLock;

        void watch();
        void dispatch();
    };

} // prv

END_BOOMER_NAMESPACE_EX(io)
