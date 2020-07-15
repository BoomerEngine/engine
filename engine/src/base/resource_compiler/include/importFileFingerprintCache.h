/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/io/include/timestamp.h"
#include "base/containers/include/hashMap.h"
#include "importFileFingerprint.h"

namespace base
{
    namespace res
    {
        //---

        struct BASE_RESOURCE_COMPILER_API ImportFileFingerprintTimestampEntry
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(ImportFileFingerprintTimestampEntry);

            uint64_t timestamp = 0;
            ImportFileFingerprint fingerprint;
        };

        struct BASE_RESOURCE_COMPILER_API ImportFileFingerprintCacheEntry
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(ImportFileFingerprintCacheEntry);

            StringBuf absolutePathUTF8; // conformed - lower case
            Array<ImportFileFingerprintTimestampEntry> timestampEntries; // sorted by time
        };

        //--
            
        // trivial "cache" for fingerprints
        class BASE_RESOURCE_COMPILER_API ImportFingerprintCache : public IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ImportFingerprintCache, IResource);

        public:
            ImportFingerprintCache();
            virtual ~ImportFingerprintCache();

            //--

            /// get number of entries in cache
            INLINE uint32_t size() const { return m_entries.size(); }

            //--

            /// clear the cache
            void clear();

            /// find entry for given path and timestamp
            bool findEntry(io::AbsolutePathView path, io::TimeStamp timestamp, ImportFileFingerprint& outFingerprint);

            /// store entry for given path and timestamp
            void storeEntry(io::AbsolutePathView path, io::TimeStamp timestamp, const ImportFileFingerprint& fingerprint);

        private:
            HashMap<StringBuf, uint32_t> m_entriesMap;
            Array<ImportFileFingerprintCacheEntry> m_entries;

            //--

            void rebuildMap();

            void conformPath(io::AbsolutePathView path, StringBuf& outPath) const;

            virtual void onPostLoad() override;
        };

        //---

    } // res
} // base