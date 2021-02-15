/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooking #]
***/

#include "build.h"
#include "cookerDependencyTracking.h"
#include "base/resource/include/depotService.h"

namespace base
{
    namespace res
    {
        
        ///--

        DependencyTracker::DependencyTracker()
        {
            m_depot = GetService<DepotService>();

            // create the root directory
            m_sourceAssetRootDir = new TrackedDepotDir;
            m_sourceAssetRootDir->depotPath = "/";
            m_sourceAssetDirs.pushBack(m_sourceAssetRootDir);

            // register to depot events
            m_events.bind(m_depot->eventKey(), EVENT_DEPOT_FILE_ADDED) = [this](StringBuf path)
            {
                notifyFileChanged(path);
            };
        }

        DependencyTracker::~DependencyTracker()
        {
            m_sourceAssetRootDir = nullptr;

            m_sourceAssetsMap.clear();
            m_sourceAssets.clearPtr();
            m_sourceAssetDirs.clearPtr();

            m_cookedAssetsMap.clear();
            m_cookedAssets.clearPtr();
        }

        bool DependencyTracker::checkUpToDate(const ResourceKey& key) CAN_YIELD
        {
            ASSERT(!key.empty());

            // get the resource entry
            // NOTE: if we are not tracking this entry will not exist (which means we don't know the tracked state yet)
            if (auto entry  = resourceEntry(key, false /* do not create if missing */))
                return checkUpToDate(*entry);
            else
                return false;
        }

        void DependencyTracker::queryFilesForReloading(Array<ResourceKey>& outReloadList)
        {
            m_changedAssetsLock.acquire();
            auto files = std::move(m_changedAssets);
            m_changedAssetsLock.release();

            for (auto file  : files)
                if (1 == file->changed.exchange(0))
                    outReloadList.pushBack(file->key);
        }

        DependencyTracker::TrackedCookedFile* DependencyTracker::resourceEntry(const ResourceKey& key, bool createIfMissing)
        {
            auto lock = CreateLock(m_cookedAssetsLock);

            // find entry
            TrackedCookedFile* ret = nullptr;
            if (m_cookedAssetsMap.find(key, ret))
                return ret;

            // create new entry
            if (!ret && createIfMissing)
            {
                TRACE_SPAM("Created tracking entry for '{}'", key);
                ret = new TrackedCookedFile;
                ret->key = key;
                m_cookedAssetsMap[key] = ret;
                m_cookedAssets.pushBack(ret);
            }

            return ret;
        }

        bool DependencyTracker::checkUpToDate(const TrackedCookedFile& entry) CAN_YIELD
        {
            auto lock = CreateLock(entry.dependenciesLock);

            // check all files
            for (auto& dep : entry.dependencies)
            {
                io::TimeStamp currentFileTimestamp;
                if (!m_depot->queryFileTimestamp(dep.file->depotPath, currentFileTimestamp))
                {
                    TRACE_WARNING("Missing source file '{}'", dep.file->depotPath);
                    return false;
                }

                if (currentFileTimestamp.value() != dep.timestamp)
                {
                    TRACE_WARNING("Source file '{}' outdated", dep.file->depotPath);
                    return false;
                }
            }

            // no dependencies were invalid
            return true;
        }

        //

        DependencyTracker::TrackedDepotFile* DependencyTracker::fileEntry_NoLock(TrackedDepotDir* parentDir, StringView fileName, bool createIfMissing)
        {
            if (fileName.empty())
                return nullptr;

            for (auto file  : parentDir->files)
                if (file->name == fileName)
                    return file;

            if (!createIfMissing)
                return nullptr;

            auto file = new TrackedDepotFile;
            file->parent = parentDir;
            file->name = StringBuf(fileName);
            file->depotPath = TempString("{}{}", parentDir->depotPath, fileName);
            parentDir->files.pushBack(file);
            m_sourceAssetsMap[file->depotPath] = file;
            m_sourceAssets.pushBack(file);
            return file;
        }

        DependencyTracker::TrackedDepotDir* DependencyTracker::childDirectory_NoLock(TrackedDepotDir* parentDir, StringView dirName, bool createIfMissing)
        {
            if (dirName.empty())
                return nullptr;

            for (auto dir  : parentDir->dirs)
                if (dir->name == dirName)
                    return dir;

            if (!createIfMissing)
                return nullptr;

            auto dir = new TrackedDepotDir;
            dir->parent = parentDir;
            dir->name = StringBuf(dirName);
            dir->depotPath = TempString("{}{}/", parentDir->depotPath, dirName);
            parentDir->dirs.pushBack(dir);
            m_sourceAssetDirs.pushBack(dir);
            return dir;
        }

        DependencyTracker::TrackedDepotFile* DependencyTracker::fileEntry(StringView filePath, bool createIfMissing)
        {
            auto lock = CreateLock(m_sourceAssetsLock);

            // find entry in the existing cache
            TrackedDepotFile* ret = nullptr;
            if (m_sourceAssetsMap.find(filePath, ret))
                return ret;

            PathEater<char> pathEater(filePath);

            // find the target directory
            auto dir  = m_sourceAssetRootDir;
            while (dir && !pathEater.endOfPath())
            {
                auto dirName = pathEater.eatDirectoryName();
                if (dirName.empty())
                    break;

                dir = childDirectory_NoLock(dir, dirName, createIfMissing);
            }

            // create the file
            if (dir)
                return fileEntry_NoLock(dir, pathEater.restOfThePath(), createIfMissing);
            else
                return nullptr;
        }

        void DependencyTracker::notifyFileChanged(const StringBuf& depotFilePath)
        {
            InplaceArray<TrackedCookedFile*, 20> changedFiles;

            if (auto fileEntry  = this->fileEntry(depotFilePath, false /* do not create if we are not tracking*/))
            {
                auto lock = CreateLock(fileEntry->usersLock);

                for (auto user  : fileEntry->users)
                    if (0 == user->changed.exchange(1))
                        changedFiles.pushBack(user);
            }

            if (!changedFiles.empty())
            {
                auto lock = CreateLock(m_changedAssetsLock);

                for (auto file  : changedFiles)
                {
                    TRACE_SPAM("Reported '{}' as changed", file->key);
                    m_changedAssets.pushBack(file);
                }
            }
        }

        void DependencyTracker::notifyDependenciesChanged(const ResourceKey& key, const Array<SourceDependency>& newDependencies)
        {
            auto resourceEntry  = this->resourceEntry(key, true /* yes do create a new entry */);
            if (resourceEntry)
            {
                Array<TrackedFileDependency> rawFileDependencies;
                rawFileDependencies.reserve(newDependencies.size());

                // prepare entries with local files
                for (auto& dep : newDependencies)
                {
                    auto depFile  = fileEntry(dep.sourcePath, true);
                    if (depFile)
                    {
                        TRACE_SPAM("Tracking '{}' dep on '{}'", key, dep.sourcePath);

                        auto& rawEntry = rawFileDependencies.emplaceBack();
                        rawEntry.file = depFile;
                        rawEntry.timestamp = dep.timestamp;
                    }
                }

                // set the new file dependencies
                {
                    auto lock = CreateLock(resourceEntry->dependenciesLock);

                    for (auto& source : resourceEntry->dependencies)
                    {
                        auto lock2 = CreateLock(source.file->usersLock);
                        source.file->users.remove(resourceEntry);
                    }

                    resourceEntry->dependencies = std::move(rawFileDependencies);

                    for (auto& source : resourceEntry->dependencies)
                    {
                        auto lock2 = CreateLock(source.file->usersLock);
                        source.file->users.insert(resourceEntry);
                    }
                }
            }
        }

    } // res
} // base


