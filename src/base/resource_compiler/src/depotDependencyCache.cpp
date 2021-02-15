/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot #]
***/

#include "build.h"
#include "depotStructure.h"
#include "depotDependencyCache.h"

namespace base
{
    namespace depot
    {

        //--

        DependencyCache::DependencyCache(DepotStructure& depot)
            : m_depot(depot)
        {}

        DependencyCache::~DependencyCache()
        {
            m_cacheMap.clearPtr();
            m_outdatedEntries.clearPtr();
        }

        bool DependencyCache::loadDependenciesNoCache(StringView depotPath, Array<res::FileLoadingDependency>& outDependencies) const
        {
            if (auto file = m_depot.createFileAsyncReader(depotPath))
            {
                res::FileLoadingContext context;
                //context.resourceLoadPath = res::ResourcePath(depotPath);
                return LoadFileDependencies(file, context, outDependencies);
            }

            return false;
        }

        const FileDependencies* DependencyCache::queryFileDependencies(StringView depotPath)
        {
            // query file timestamp
            io::TimeStamp timestamp;
            if (!m_depot.queryFileTimestamp(depotPath, timestamp))
                return nullptr; // file does not exist

            // find existing entry
            {
                auto lock = CreateLock(m_cacheMapLock);
                CacheEntry* entry = nullptr;
                if (m_cacheMap.find(depotPath, entry))
                    if (entry->timestamp == timestamp)
                        return &entry->deps;
            }

            // load dependencies
            Array<res::FileLoadingDependency> dependencies;
            if (!loadDependenciesNoCache(depotPath, dependencies))
                return nullptr;

            // create entry
            auto entry = new CacheEntry();
            entry->timestamp = timestamp;
            entry->path = StringBuf(depotPath);
            entry->deps.timestamp = timestamp;
            entry->deps.dependencies = std::move(dependencies);

            {
                auto lock = CreateLock(m_cacheMapLock);
                CacheEntry*& mapEntry = m_cacheMap[entry->path];
                if (mapEntry)
                    m_outdatedEntries.pushBack(mapEntry);
                mapEntry = entry;
            }

            return &entry->deps;
        }

        //--

    } // depot
} // base
