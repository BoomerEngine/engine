/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "base/containers/include/hashMap.h"
#include "importFileFingerprint.h"
#include "base/io/include/timestamp.h"

namespace base
{
    namespace res
    {
        //--
        
        /// repository of source assets (basic loader + cache)
        class BASE_RESOURCE_COMPILER_API SourceAssetRepository : public NoCopy
        {
        public:
            SourceAssetRepository(ImportFileService* fileService, uint64_t maxCachedSourceAssetsMemoryBudget = 0);
            virtual ~SourceAssetRepository();

            ///---

            /// clear the internal cache
            void clearCache();

            /// check if file exists
            bool fileExists(StringView assetImportPath) const;

            /// load raw data for an asset, usually not cached
            Buffer loadSourceFileContent(StringView assetImportPath, io::TimeStamp& outTimestamp, ImportFileFingerprint& outFingerprint);

            /// load source asset
            SourceAssetPtr loadSourceAsset(StringView assetImportPath, io::TimeStamp& outTimestamp, ImportFileFingerprint& outFingerprint);

            /// load/create base resource configuration
            ResourceConfigurationPtr compileBaseResourceConfiguration(StringView assetImportPath, SpecificClassType<ResourceConfiguration> configurationClass);

            // check status of a file
            CAN_YIELD SourceAssetStatus checkFileStatus(StringView assetImportPath, const io::TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownFingerprint, IProgressTracker* progress);
            
            ///---

        private:
            Mutex m_lock;

            ImportFileService* m_fileService = nullptr;

            struct CacheEntry : NoCopy
            {
                StringBuf assetImportPath;
                SourceAssetPtr asset;
                io::TimeStamp timestamp;
                ImportFileFingerprint fingerprint;
                uint32_t lruTick = 0;
                uint64_t memorySize = 0;
            };

            uint64_t m_memoryBudgetSize = 0;
            uint32_t m_lruTick = 0;

            uint64_t m_maxMemorySize = 0;
            uint64_t m_totalMemorySize = 0;
            Array<CacheEntry*> m_cacheEntries;
            HashMap<StringBuf, CacheEntry*> m_cacheEntriesMap;

            //--

            void ensureMemoryForAsset(uint64_t neededMemorySize);

        };

        //--

    } // res
} // base