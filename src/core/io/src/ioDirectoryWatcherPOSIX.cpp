/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\watcher\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "ioDirectoryWatcherPOSIX.h"
#include "ioFileIteratorPOSIX.h"
#include "absolutePath.h"

#include "core/system/include/scopeLock.h"
#include "core/system/include/thread.h"

#include <sys/types.h>
#include <sys/inotify.h>

namespace base
{
    namespace io
    {
        namespace prv
        {

            POSIXDirectoryWatcher::POSIXDirectoryWatcher(StringView rootPath)
            {
                // create the notify interface
                m_masterHandle = inotify_init1(IN_NONBLOCK);
                if (m_masterHandle >= 0)
                {
                    TRACE_SPAM("Created directory watcher for '{}'", rootPath);
                }
                else
                {
                    TRACE_ERROR("Failed to create directory watcher");
                }

                // create the watcher thread
                ThreadSetup setup;
                setup.m_name = "DirectoryWatcher";
                setup.m_priority = Priority::AboveNormal;
                setup.m_function = [this]() { watch(); };
                m_localThread.init(setup);

                // start monitoring the root path
                monitorPath(rootPath);
            }

            POSIXDirectoryWatcher::~POSIXDirectoryWatcher()
            {
                // close the master handle
                if (m_masterHandle >= 0)
                {
                    // unmonitor all paths
                    {
                        ScopeLock<SpinLock> lock(m_mapLock);

                        for (auto watchId  : m_handleToPath.keys())
                            inotify_rm_watch(m_masterHandle, watchId);

                        m_handleToPath.clear();
                        m_pathToHandle.clear();
                    }

                    // close the master handle
                    close(m_masterHandle);
                    m_masterHandle = -1;
                }

                // stop thread
                m_localThread.close();
            }

            void POSIXDirectoryWatcher::attachListener(IDirectoryWatcherListener* listener)
            {
                ScopeLock<Mutex> lock(m_listenersLock);
                m_listeners.pushBack(listener);
            }

            void POSIXDirectoryWatcher::dettachListener(IDirectoryWatcherListener* listener)
            {
                ScopeLock<Mutex> lock(m_listenersLock);
                auto index  = m_listeners.find(listener);
                if (index != -1)
                    m_listeners[index] = nullptr;
            }

            void POSIXDirectoryWatcher::monitorPath(StringView path)
            {
                // we can create the watch only if we are initialized properly
                if (m_masterHandle >= 0)
                {
                    // create the watcher
                    auto watcherId  = inotify_add_watch(m_masterHandle, path.ansi_str().c_str(), IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVE | IN_MODIFY | IN_ATTRIB);
                    if (watcherId != -1)
                    {
                        TRACE_SPAM("Added directory watch to '{}', handle: {}", path, watcherId);

                        // add to map
                        {
                            ScopeLock<SpinLock> lock(m_mapLock);
                            m_handleToPath.set(watcherId, path);
                            m_pathToHandle.set(path.view().calcCRC64(), watcherId);
                        }

                        // monitor the existing sub directories as well
                        for (POSIXFileIterator it(path, L"*.", false, true); it; ++it)
                        {
                            auto subDirPath  = path.addDir(it.fileName());
                            monitorPath(subDirPath);
                        }
                    }
                }
            }

            void POSIXDirectoryWatcher::unmonitorPath(StringView path)
            {
                // un monitor the existing sub directories as well
                for (POSIXFileIterator it(path, L"*.", false, true); it; ++it)
                {
                    auto subDirPath  = path.addDir(it.fileName());
                    unmonitorPath(subDirPath);
                }

                // remove current watcher
                ScopeLock<SpinLock> lock(m_mapLock);

                int watcherId = 0;
                auto pathHash  = path.view().calcCRC64();
                if (m_pathToHandle.find(pathHash, watcherId))
                {
                    // remove from tables
                    TRACE_SPAM("Removed directory watcher at '{}' ({})", path.c_str(), watcherId);
                    m_pathToHandle.remove(pathHash);
                    m_handleToPath.remove(watcherId);

                    // remove from system
                    inotify_rm_watch(m_masterHandle, watcherId);
                }
            }

            void POSIXDirectoryWatcher::watch()
            {
                for (;;)
                {
                    // nothing to read
                    if (m_masterHandle <= 0)
                        break;

                    // read data from the crap
                    auto dataSize  = read(m_masterHandle, m_buffer, BUF_LEN);
                    if (dataSize < 0)
                    {
                        if (errno == EAGAIN)
                        {
                            Sleep(10);
                            continue;
                        }

                        TRACE_ERROR("Read error in the directory watcher data stream: {}", errno);
                        break;
                    }

                    // prepare tables
                    m_tempEvents.reset();
                    m_tempAddedDirectories.reset();
                    m_tempRemovedDirectories.reset();

                    // process data
                    auto cur  = &m_buffer[0];
                    auto end  = &m_buffer[dataSize];
                    while (cur < end)
                    {
                        // get event
                        auto &evt = *(struct inotify_event *) cur;

                        // prevent modifications of the map table
                        m_mapLock.acquire();

                        // identify the target path entry
                        auto pathEntry  = m_handleToPath.find(evt.wd);
                        if (!pathEntry)
                        {
                            TRACE_INFO("IO event at unrecognized path, ID {}, '{}'", evt.wd, evt.name);
                            m_mapLock.release();
                            break;
                        }

                        TRACE_INFO("IO watcher: '{}' '{}' {}", pathEntry->c_str(), evt.name, Hex(evt.mask));

                        // self deleted
                        if (evt.mask & (IN_DELETE_SELF | IN_MOVE_SELF))
                        {
                            // find path
                            TRACE_INFO("Observed directory '{}' self deleted", pathEntry->c_str());
                            auto& info = m_tempEvents.emplaceBack();
                            info.m_type = DirectoryWatcherEventType::DirectoryRemoved;
                            info.m_path = *pathEntry;

                            // unmap
                            m_pathToHandle.remove(pathEntry->view().calcCRC64());
                            m_handleToPath.remove(evt.wd);
                            m_mapLock.release();
                        }
                        else
                        {
                            // format full path
                            bool isDir = (0 != (evt.mask & IN_ISDIR));
                            auto shortPath  = &evt.name[0];
                            auto fullPath  = isDir ? pathEntry->addDir(shortPath) : pathEntry->addFile(shortPath);
                            m_mapLock.release();

                            // stuff was created
                            if (evt.mask & IN_CREATE)
                            {
                                // if a directory is added make sure to monitor it as well
                                if (isDir)
                                {
                                    m_tempAddedDirectories.pushBack(fullPath);

                                    auto &evt = m_tempEvents.emplaceBack();
                                    evt.m_type = isDir ? DirectoryWatcherEventType::DirectoryAdded : DirectoryWatcherEventType::FileAdded;
                                    evt.m_path = fullPath;
                                }
                                else
                                {
                                    // remember the file around
                                    m_filesCreatedButNotYetClosed.pushBackUnique(fullPath);
                                }
                            }

                            // writable file was closed
                            if (evt.mask & IN_CLOSE_WRITE)
                            {
                                // report event only if the fle was previously modified
                                if (m_filesCreatedButNotYetClosed.remove(fullPath))
                                {
                                    auto &evt = m_tempEvents.emplaceBack();
                                    evt.m_type = DirectoryWatcherEventType::FileAdded;
                                    evt.m_path = fullPath;
                                }

                                // just modified
                                if (m_filesModifiedButNotYetClosed.remove(fullPath))
                                {
                                    auto &evt = m_tempEvents.emplaceBack();
                                    evt.m_type = DirectoryWatcherEventType::FileContentChanged;
                                    evt.m_path = fullPath;
                                }
                            }

                            // file was moved
                            if (evt.mask & IN_MOVED_TO)
                            {
                                // if a directory is added make sure to monitor it as well
                                if (isDir)
                                    m_tempAddedDirectories.pushBack(fullPath);

                                // create direct event
                                auto &evt = m_tempEvents.emplaceBack();
                                evt.m_type = isDir ? DirectoryWatcherEventType::DirectoryAdded : DirectoryWatcherEventType::FileAdded;
                                evt.m_path = fullPath;
                            }

                            // stuff was removed
                            if (evt.mask & (IN_DELETE | IN_MOVED_FROM))
                            {
                                // if a directory is added make sure to monitor it as well
                                if (isDir)
                                    m_tempRemovedDirectories.pushBack(fullPath);

                                // create event
                                auto &evt = m_tempEvents.emplaceBack();
                                evt.m_type = isDir ? DirectoryWatcherEventType::DirectoryRemoved : DirectoryWatcherEventType::FileRemoved;
                                evt.m_path = fullPath;
                            }

                            // stuff was changed
                            if (evt.mask & (IN_MODIFY))
                            {
                                if (!isDir)
                                {
                                    // watch for file changes
                                    m_filesModifiedButNotYetClosed.pushBackUnique(fullPath);
                                }
                            }

                            // metadata changed
                            if (evt.mask & (IN_ATTRIB))
                            {
                                auto &evt = m_tempEvents.emplaceBack();
                                evt.m_type = DirectoryWatcherEventType::FileMetadataChanged;
                                evt.m_path = fullPath;
                            }
                        }

                        // advance
                        cur += sizeof(struct inotify_event) + evt.len;
                    }

                    // unmonitor directories that got removed
                    for (auto &path : m_tempRemovedDirectories)
                        unmonitorPath(path);

                    // start monitoring directories that go added
                    for (auto &path : m_tempAddedDirectories)
                        monitorPath(path);

                    // lock access to this thread only
                    ScopeLock<Mutex> lock(m_listenersLock);

                    // send events to the listeners
                    for (auto &evt : m_tempEvents)
                    {
                        for (auto listener  : m_listeners)
                        {
                            if (listener != nullptr)
                                listener->handleEvent(evt);
                        }
                    }

                    // remove the empty listeners
                    m_listeners.remove(nullptr);
                }
            }

        } // prv
    } // io
} // base