/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\watcher #]
***/

#include "build.h"
#include "ioSystem.h"
#include "ioDirectoryWatcher.h"
#include "ioDirectoryMonitor.h"

namespace base
{
    namespace io
    {
        //--

        namespace prv
        {
            /// a watcher listener that sets the atomic flag
            class AtomicTogglerWatcher : public IDirectoryWatcherListener
            {
                RTTI_DECLARE_POOL(POOL_IO)

            public:
                AtomicTogglerWatcher(std::atomic<uint32_t>& flag)
                    : m_flag(&flag)
                {}

                virtual void handleEvent(const DirectoryWatcherEvent& evt) override final
                {
                    m_flag->exchange(1); // signal
                }

            private:
                std::atomic<uint32_t>* m_flag;
            };

        } // prv

        //--

        DirectoryMonitor::DirectoryMonitor(const StringBuf& path, bool recursive)
            : m_path(path)
            , m_isRecursive(recursive)
            , m_modified(0)
        {
            // create the watcher
            m_watcher = CreateDirectoryWatcher(path);
            if (m_watcher)
            {
                // create and attach listener
                m_listener = new prv::AtomicTogglerWatcher(m_modified);
                m_watcher->attachListener(m_listener);
            }
        }

        DirectoryMonitor::~DirectoryMonitor()
        {
            // destroy watcher
            if (m_watcher)
            {
                // detach listener
                if (m_listener != nullptr)
                {
                    m_watcher->dettachListener(m_listener);
                    delete m_listener;
                    m_listener = nullptr;
                }

                // destroy watcher
                m_watcher.reset();
            }

        }

        bool DirectoryMonitor::testAndReset()
        {
            return 1 == m_modified.exchange(0);
        }

        //--

    } // io
} // base