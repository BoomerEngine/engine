/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once
#include "base/containers/include/hashMap.h"

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
            bool fileExists(StringView<char> assetImportPath, uint64_t* outCRC=nullptr) const;

            /// load raw data for an asset, usually not cached
            Buffer loadSourceFileContent(StringView<char> assetImportPath, uint64_t& outCRC);

            /// load source asset
            SourceAssetPtr loadSourceAsset(StringView<char> assetImportPath, SpecificClassType<ISourceAsset> contentType, uint64_t& outCRC);

            ///---

        private:
            Mutex m_lock;

            ImportFileService* m_fileService = nullptr;

            struct CacheEntry : NoCopy
            {
                StringBuf assetImportPath;
                SourceAssetPtr asset;
                uint64_t crc = 0;
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