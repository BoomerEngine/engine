/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedDepot.h"
#include "managedDirectory.h"
#include "managedThumbnails.h"

#include "editorService.h"

#include "base/app/include/localServiceContainer.h"
#include "base/app/include/localService.h"
#include "base/system/include/thread.h"

namespace ed
{
    //--

    IManagedDepotListener::IManagedDepotListener()
        : m_depot(nullptr)
    {
        m_depot = &GetService<Editor>()->managedDepot();
        if (m_depot)
            m_depot->registerListener(this);
    }

    IManagedDepotListener::~IManagedDepotListener()
    {
        if (m_depot)
        {
            m_depot->unregisterListener(this);
            m_depot = nullptr;
        }
    }

    void IManagedDepotListener::detach()
    {
        m_depot = nullptr;
    }

    //--

    ManagedDepot::ManagedDepot(depot::DepotStructure& loader, ConfigGroup config)
        : m_loader(loader)
        , m_config(config)
    {
        // create thumbnail service
        m_thumbnailHelper = CreateUniquePtr<ManagedThumbnailHelper>(loader);

        // create the root directory, it never goes away
        m_root = CreateSharedPtr<ManagedDirectory>(this, nullptr, "Assets", "", true);

        // register a listener for depot changes
        loader.attachObserver(this);
    }

    ManagedDepot::~ManagedDepot()
    {
        // stop any thumbnail jobs
        m_thumbnailHelper.reset();

        // detach file system observer
        m_loader.detttachObserver(this);

        // detach all listeners
        m_listenersLock.acquire();
        auto listeners = std::move(m_listeners);
        m_listenersLock.release();
        for (auto listener  : listeners)
            if (listener)
                listener->detach();

        // release directory structure
        m_root.reset();
    }

    void ManagedDepot::populate()
    {
        base::ScopeTimer timer;
        m_root->populate();
        TRACE_WARNING("Depot populated in {}", timer);
    }        

    ManagedDirectory* ManagedDepot::createPath(StringView<char> depotPath, const char** leftOver)
    {
        // get the root path
        auto curDir = root();
        if (!curDir)
            return nullptr;

        // split into directories, create as we go into the path
        auto path  = depotPath.data();
        auto start  = path;
        while (*path)
        {
            // split after each path breaker
            if (*path == '\\' || *path == '/')
            {
                // extract directory name
                if (path > start)
                {
                    auto dirName = StringView<char>(start, path);

                    // use existing directory or create a new one
                    auto childDir = curDir->directory(dirName);
                    if (!childDir)
                    {
                        childDir = curDir->createDirectory(dirName);
                        if (!childDir)
                        {
                            TRACE_ERROR("Creating path '{}' failed: failed to create directory '{}'.", path, dirName);
                            return ManagedDirectoryPtr();
                        }
                    }

                    // use this directory next time
                    curDir = childDir;
                }

                // advance
                start = path+1;
            }

            ++path;
        }

        // Get file name
        if (leftOver)
            *leftOver = start;

        // Return final directory
        return curDir;
    }

    ManagedDirectory* ManagedDepot::findPath(StringView<char> depotPath, const char** leftOver /*= nullptr*/) const
    {
        auto curDir = root();
        if (!curDir)
            return nullptr;

        // split into directories, create as we go into the path
        const char* start = depotPath.data();
        const char* path = start;
        while (*path)
        {
            // split after each path breaker
            if (*path == '\\' || *path == '/')
            {
                // extract directory name
                if (path > start)
                {
                    auto dirName = StringView<char>(start, path);
                    if (dirName == ".boomer")
                        return nullptr;

                    // use existing directory or create a new one
                    auto childDir = curDir->directory(dirName);
                    if (!childDir)
                        return nullptr;

                    curDir = childDir;
                }

                // advance
                start = path + 1;
            }

            ++path;
        }

        // Get file name
        if (leftOver)
            *leftOver = start;

        // Return final directory
        return curDir;
    }

    ManagedFile* ManagedDepot::findManagedFile(StringView<char> depotPath) const
    {
        if (depotPath)
        {
            const char* fileName = nullptr;
            if (auto sectorDir = findPath(depotPath, &fileName))
                return sectorDir->file(fileName);
        }

        return nullptr;
    }

    static bool InterestedInFile(StringView<char> txt)
    {
        auto ext = txt.afterLast(".");
        if (0 == ext.caseCmp("bak")) return false;
        if (0 == ext.caseCmp("thumb")) return false;
        return true;
    }

    void ManagedDepot::dispatchEvents()
    {
        // swap
        m_internalEventsLock.acquire();
        auto events = std::move(m_internalEvents);
        m_internalEventsLock.release();

        // process events
        for (auto& evt : events)
        {
            if (!InterestedInFile(evt.depotPath))
                continue;

            switch (evt.type)
            {
                case InternalFileEventType::FileAdded:
                {
                    const char* fileName = nullptr;
                    auto targetDir = findPath(evt.depotPath, &fileName);
                    if (targetDir)
                    {
                        if (targetDir->notifyDepotFileCreated(fileName))
                        {
                            if (auto targetFile = targetDir->file(fileName, true))
                                dispatchEvent(targetFile, ManagedDepotEvent::FileCreated);
                        }
                    }

                    break;
                }

                case InternalFileEventType::FileRemoved:
                {
                    const char* fileName = nullptr;
                    auto targetDir = findPath(evt.depotPath, &fileName);
                    if (targetDir)
                    {
                        if (auto targetFile = targetDir->file(fileName, true))
                            dispatchEvent(targetFile, ManagedDepotEvent::FileDeleted);

                        targetDir->notifyDepotFileDeleted(fileName);
                    }
                    break;
                }

                case InternalFileEventType::DirAdded:
                {
                    const char* dirName = nullptr;
                    auto targetDir = findPath(evt.depotPath, &dirName);
                    if (targetDir)
                    {
                        if (targetDir->notifyDepotDirCreated(dirName))
                        {
                            if (auto childDir = targetDir->directory(dirName, true))
                                dispatchEvent(childDir, ManagedDepotEvent::DirCreated);
                        }
                    }
                    break;
                }

                case InternalFileEventType::DirRemoved:
                {
                    const char* dirName = nullptr;
                    auto targetDir = findPath(evt.depotPath, &dirName);
                    if (targetDir)
                    {
                        if (auto childDir = targetDir->directory(dirName, true))
                            dispatchEvent(childDir, ManagedDepotEvent::DirDeleted);

                        if (targetDir->notifyDepotDirDeleted(dirName))
                        {
                        }
                    }
                    break;
                }
            }

        }
    }

    void ManagedDepot::dispatchEvent(ManagedItem* item, ManagedDepotEvent eventType)
    {
        for (uint32_t i = 0; i < m_listeners.size(); ++i)
        {
            auto listener = m_listeners[i];
            if (listener)
                listener->managedDepotEvent(item, eventType);
        }
    }

    void ManagedDepot::toogleDirectoryBookmark(ManagedDirectory* dir, bool state)
    {
        bool saveConfig = false;

        if (state && m_bookmarkedDirectories.insert(dir))
        {
            dispatchEvent(dir, ManagedDepotEvent::DirBookmarkChanged);
            saveConfig = true;
        }
        else if (!state && m_bookmarkedDirectories.remove(dir))
        {
            dispatchEvent(dir, ManagedDepotEvent::DirBookmarkChanged);
            saveConfig = true;
        }

        if (saveConfig)
        {
            Array<StringBuf> bookmarkedDirectories;
            bookmarkedDirectories.reserve(m_bookmarkedDirectories.size());

            for (auto* dir : m_bookmarkedDirectories.keys())
                bookmarkedDirectories.pushBack(dir->depotPath());

            m_config.write("Bookmarks", bookmarkedDirectories);
        }            
    }

    void ManagedDepot::toogleFileModified(ManagedFile* file, bool state)
    {
        if (state && m_modifiedFiles.insert(file))
            dispatchEvent(file, ManagedDepotEvent::FileModifiedChanged);
        else if (!state && m_modifiedFiles.remove(file))
            dispatchEvent(file, ManagedDepotEvent::FileModifiedChanged);
    }

    void ManagedDepot::registerListener(IManagedDepotListener* listener)
    {
        ScopeLock<Mutex> lock(m_listenersLock);

        ASSERT(listener != nullptr);
        ASSERT(!m_listeners.contains(listener));
        m_listeners.pushBack(listener);
    }

    void ManagedDepot::unregisterListener(IManagedDepotListener* listener)
    {
        ASSERT(listener != nullptr);

        ScopeLock<Mutex> lock(m_listenersLock);

        auto index = m_listeners.find(listener);
        ASSERT(index != INDEX_NONE);

        if (index != INDEX_NONE)
            m_listeners[index] = nullptr;
    }

    //---

    void ManagedDepot::restoreConfiguration()
    {
        // load list of bookmarked directories
        {
            auto bookmarkedDirectories = m_config.readOrDefault< Array<StringBuf> >("Bookmarks");
            m_bookmarkedDirectories.clear();

            for (auto& path : bookmarkedDirectories)
            {
                auto dir = findPath(path);
                if (dir)
                {
                    m_bookmarkedDirectories.insert(dir);
                    dir->bookmark(true);
                }
            }
        }
    }

    //---

    void ManagedDepot::notifyFileChanged(StringView<char> depotFilePath)
    {

    }

    void ManagedDepot::notifyFileAdded(StringView<char> depotFilePath)
    {
        auto lock = CreateLock(m_internalEventsLock);
        auto& entry = m_internalEvents.emplaceBack();
        entry.depotPath = StringBuf(depotFilePath);
        entry.type = InternalFileEventType::FileAdded;
    }

    void ManagedDepot::notifyFileRemoved(StringView<char> depotFilePath)
    {
        auto lock = CreateLock(m_internalEventsLock);
        auto& entry = m_internalEvents.emplaceBack();
        entry.depotPath = StringBuf(depotFilePath);
        entry.type = InternalFileEventType::FileRemoved;
    }

    void ManagedDepot::notifyDirAdded(StringView<char> depotPath)
    {
        // the directory depot paths must end with the path separators
        if (!depotPath.endsWith("/") && !depotPath.endsWith("\\"))
        {
            TRACE_WARNING("Path '{}' is not valid depot directory path", depotPath);
        }
        else
        {
            // add internal FS event to process later
            auto lock = CreateLock(m_internalEventsLock);
            auto& entry = m_internalEvents.emplaceBack();
            entry.depotPath = StringBuf(depotPath.leftPart(depotPath.length() - 1));
            entry.type = InternalFileEventType::DirAdded;
        }
    }

    void ManagedDepot::notifyDirRemoved(StringView<char> depotPath)
    {
        // the directory depot paths must end with the path separators
        if (!depotPath.endsWith("/") && !depotPath.endsWith("\\"))
        {
            TRACE_WARNING("Path '{}' is not valid depot directory path", depotPath);
        }
        else
        {
            // add internal FS event to process later
            auto lock = CreateLock(m_internalEventsLock);
            auto& entry = m_internalEvents.emplaceBack();
            entry.depotPath = StringBuf(depotPath.leftPart(depotPath.length() - 1));
            entry.type = InternalFileEventType::DirRemoved;
        }
    }

    //---

    /*void ManagedDepot::onSaveConfiguration()
    {
        // save list of bookmarked directories
        {

        }
    }*/

    //---

} // depot


