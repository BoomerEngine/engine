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
#include "base/ui/include/uiElementConfig.h"

namespace ed
{
    //--

    ManagedDepot::ManagedDepot(depot::DepotStructure& depot)
        : m_depot(depot)
    {
        // event key for listening our events
        m_eventKey = MakeUniqueEventKey("ManagedDepot");

        // create thumbnail service
        //m_thumbnailHelper = CreateUniquePtr<ManagedThumbnailHelper>(loader);

        // create the root directory, it never goes away
        m_root = CreateSharedPtr<ManagedDirectory>(this, nullptr, "root", "/");

        // bind events
        m_depotEvents.bind(depot.eventKey(), EVENT_DEPOT_FILE_ADDED) = [this](StringBuf path) {
            handleDepotFileNotificataion(path);
        };
        m_depotEvents.bind(depot.eventKey(), EVENT_DEPOT_FILE_CHANGED) = [this](StringBuf path) {
            handleDepotFileNotificataion(path);
        };
        m_depotEvents.bind(depot.eventKey(), EVENT_DEPOT_FILE_REMOVED) = [this](StringBuf path) {
            handleDepotFileNotificataion(path);
        };
        m_depotEvents.bind(depot.eventKey(), EVENT_DEPOT_DIRECTORY_ADDED) = [this](StringBuf path) {
            handleDepotFileNotificataion(path);
        };
        m_depotEvents.bind(depot.eventKey(), EVENT_DEPOT_DIRECTORY_REMOVED) = [this](StringBuf path) {
            handleDepotFileNotificataion(path);
        };

        // reload event
        m_depotEvents.bind(depot.eventKey(), EVENT_DEPOT_FILE_RELOADED) = [this](StringBuf path)
        {
            if (auto file = ManagedFilePtr(AddRef(findManagedFile(path))))
            {
                DispatchGlobalEvent(eventKey(), EVENT_MANAGED_DEPOT_FILE_RELOADED, file);
                DispatchGlobalEvent(file->eventKey(), EVENT_MANAGED_FILE_RELOADED);
            }
        };
    }

    ManagedDepot::~ManagedDepot()
    {
        m_depotEvents.clear();
        m_root.reset();
    }

    void ManagedDepot::populate()
    {
        ScopeTimer timer;
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

    void ManagedDepot::toogleDirectoryBookmark(ManagedDirectory* dir, bool state)
    {
        if (state && m_bookmarkedDirectories.insert(dir))
            DispatchGlobalEvent(m_eventKey, EVENT_MANAGED_DEPOT_DIRECTORY_BOOKMARKED, ManagedDirectoryPtr(AddRef(dir)));
        else if (!state && m_bookmarkedDirectories.remove(dir))
            DispatchGlobalEvent(m_eventKey, EVENT_MANAGED_DEPOT_DIRECTORY_UNBOOKMARKED, ManagedDirectoryPtr(AddRef(dir)));
    }

    void ManagedDepot::toogleFileModified(ManagedFile* file, bool state)
    {
        if (state && m_modifiedFiles.insert(file))
        {
            DispatchGlobalEvent(m_eventKey, EVENT_MANAGED_DEPOT_FILE_MODIFIED, file->depotPath());
            DispatchGlobalEvent(file->eventKey(), EVENT_MANAGED_FILE_MODIFIED);
        }
        else if (!state && m_modifiedFiles.remove(file))
        {
            DispatchGlobalEvent(m_eventKey, EVENT_MANAGED_DEPOT_FILE_UNMODIFIED, file->depotPath());
            DispatchGlobalEvent(file->eventKey(), EVENT_MANAGED_FILE_UNMODIFIED);
        }
    }

    //---

    void ManagedDepot::configLoad(const ui::ConfigBlock& block)
    {
        m_bookmarkedDirectories.clear();

        auto bookmarkedDirectories = block.readOrDefault<Array<StringBuf>>("Bookmarks");
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

    void ManagedDepot::configSave(const ui::ConfigBlock& block)
    {
        {
            Array<StringBuf> bookmarkedDirectories;
            bookmarkedDirectories.reserve(m_bookmarkedDirectories.size());

            for (auto* dir : m_bookmarkedDirectories.keys())
                bookmarkedDirectories.pushBack(dir->depotPath());

            block.write("Bookmarks", bookmarkedDirectories);
        }

        {

        }
    }

    //--

    void ManagedDepot::update()
    {
        processDepotFileNotificataion();
    }

    //---

    base::ConfigProperty<double> cvManagedDepotDirectoryRefreshTimeout("Editor.ManagedDepot", "DirectoryRefreshTimeout", 0.5);

    void ManagedDepot::handleDepotFileNotificataion(StringView<char> path)
    {
        auto parentDirPath = StringBuf(path.beforeLast("/"));
        TRACE_INFO("ManagedDepot: Reported changes in directory '{}'", parentDirPath);
        m_modifiedDirectories[parentDirPath] = NativeTimePoint::Now() + cvManagedDepotDirectoryRefreshTimeout.get();
    }

    void ManagedDepot::processDepotFileNotificataion()
    {
        InplaceArray<StringBuf, 20> finishedDirs;

        for (auto pair : m_modifiedDirectories.pairs())
            if (pair.value.reached())
                finishedDirs.emplaceBack(pair.key);

        for (const auto& dirPath : finishedDirs)
        {
            m_modifiedDirectories.remove(dirPath);

            if (auto* dir = findPath(TempString("{}/", dirPath)))
            {
                TRACE_INFO("ManagedDepot: refreshing content of directory '{}' ({})", dirPath, dir->depotPath());
                dir->populate();
            }
        }
    }

    //---

} // depot


