/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/io/include/timestamp.h"
#include "base/containers/include/hashMap.h"
#include "base/fibers/include/fiberSystem.h"
#include "importFileFingerprint.h"

namespace base
{
    namespace res
    {
        //--

        class ImportFingerprintCache;

        //--

        // a simple helper service that can compute fingerprints of files on disk
        class BASE_RESOURCE_COMPILER_API ImportFileFingerprintService : public base::app::ILocalService
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ImportFileFingerprintService, base::app::ILocalService);

        public:
            ImportFileFingerprintService();
            virtual ~ImportFileFingerprintService();

            //--

            /// compute file fingerprint of given file
            /// NOTE: this fill yield current fiber until results are available
            CAN_YIELD FingerpintCalculationStatus calculateFingerprint(StringView<wchar_t> absolutePath, bool background, IProgressTracker* progress, ImportFileFingerprint& outFingerprint);

            //---

        private:
            static const uint32_t HEADER_MAGIC = 0x43524343;//'CRCC';

            //--

            Mutex m_cacheLock;
            RefPtr<ImportFingerprintCache> m_cache;

            io::AbsolutePath m_cacheFilePath;
            NativeTimePoint m_nextCacheWriteCheck;
            std::atomic<uint32_t> m_hasNewCacheEntries = 0;

            //--

            // a job, either loading or baking
            struct CacheJob : public IReferencable
            {
                io::AbsolutePath path;
                io::TimeStamp timestamp;
                ImportFileFingerprint fingerprint;
                FingerpintCalculationStatus status = FingerpintCalculationStatus::OK;
                fibers::WaitCounter signal;
            };

            // synchronization for blobs being loaded/saved
            SpinLock m_activeJobMapLock;
            HashMap<io::AbsolutePath, RefWeakPtr<CacheJob>> m_activeJobMap;

            //--

            virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override;
            virtual void onShutdownService() override;
            virtual void onSyncUpdate() override;

            void saveCacheIfDirty();

            //--
        };

    } // io
} // base