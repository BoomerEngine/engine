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
#include "base/system/include/thread.h"
#include "base/system/include/timing.h"

namespace base
{
    namespace io
    {
        namespace prv
        {

            /// POSIX implementation of the watcher
            class WinDirectoryWatcher : public IDirectoryWatcher
            {
            public:
                WinDirectoryWatcher(StringView<wchar_t> rootPath);
                virtual ~WinDirectoryWatcher();

                //! attach listener
                virtual void attachListener(IDirectoryWatcherListener* listener) override;
                virtual void dettachListener(IDirectoryWatcherListener* listener) override;

            private:
                AbsolutePath m_watchedPath;

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
                    AbsolutePath path;
                    NativeTimePoint time;
                    NativeTimePoint expires;
                };

                Array<PendingModification> m_pendingModifications;
                SpinLock m_pendingModificationsLock;

                void watch();
                void dispatch();
            };

        } // prv
    } // io
} // base