/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot #]
***/

#pragma once

#include "base/system/include/atomic.h"
#include "base/resource/include/resourceLoader.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceFileLoader.h"
#include "base/io/include/timestamp.h"

namespace base
{
    namespace depot
    {
        //--

        // file dependencies
        struct FileDependencies
        {
            io::TimeStamp timestamp;
            Array<res::FileLoadingDependency> dependencies;
        };
        
        //--

        class BASE_RESOURCE_COMPILER_API DependencyCache : public NoCopy
        {
        public:
            DependencyCache(DepotStructure& depot);
            ~DependencyCache();

            // TODO: load/save cache

            // get loading dependencies of file
            const FileDependencies* queryFileDependencies(StringView depotPath);

        private:
            DepotStructure& m_depot;

            struct CacheEntry
            {
                RTTI_DECLARE_POOL(POOL_MANAGED_DEPOT);

            public:
                io::TimeStamp timestamp;
                StringBuf path;
                FileDependencies deps;
            };

            SpinLock m_cacheMapLock;
            HashMap<StringBuf, CacheEntry*> m_cacheMap;
            Array<CacheEntry*> m_outdatedEntries;

            //--

            bool loadDependenciesNoCache(StringView depotPath, Array<res::FileLoadingDependency>& outDependencies) const;

            //--
            
        };

        //--

    } // depot
} // base