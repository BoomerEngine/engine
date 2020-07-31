/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "importer.h"

#include "base/io/include/absolutePath.h"
#include "base/app/include/localService.h"
#include "base/socket/include/tcpServer.h"
#include "base/resource/include/resourceLoader.h"
#include "base/system/include/semaphoreCounter.h"

namespace base
{
    namespace res
    {
        //--

        class Importer;
        class ImportSaverThread;
        class ImportQueueDepotChecker;
        struct ImportJobInfo;

        //--

        /// notification interface for the queue
        class BASE_RESOURCE_COMPILER_API IImportQueueCallbacks : public NoCopy
        {
        public:
            virtual ~IImportQueueCallbacks();

            virtual void queueJobAdded(const ImportJobInfo& info) {};
            virtual void queueJobStarted(StringView<char> depotPath) {};
            virtual void queueJobFinished(StringView<char> depotPath, ImportStatus status, double timeTaken) {};
            virtual void queueJobProgressUpdate(StringView<char> depotPath, uint64_t currentCount, uint64_t totalCount, StringView<char> text) {};
        };

        //--

        /// helper class to retrieve existing content of a resource
        class BASE_RESOURCE_COMPILER_API IImportDepotLoader : public NoCopy
        {
        public:
            virtual ~IImportDepotLoader();

            // load metadata of existing resource, should not be cached or reused
            virtual MetadataPtr loadExistingMetadata(StringView<char> depotPath) const = 0;

            // load existing content of a resource, should not be cached or reused
            virtual ResourcePtr loadExistingResource(StringView<char> depotPath) const = 0;

            // check if file exists
            virtual bool depotFileExists(StringView<char> depotPath) const = 0;

            // find depot file
            virtual bool depotFindFile(StringView<char> depotPath, StringView<char> fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const = 0;
        };

        //--

        /// a helper class that processes the list of import jobs
        class BASE_RESOURCE_COMPILER_API ImportQueue : public NoCopy
        {
        public:
            ImportQueue(SourceAssetRepository* assets, IImportDepotLoader* loader, ImportSaverThread* saver, IImportQueueCallbacks* callbacks);
            ~ImportQueue();

            //--

            /// add a import job to the queue
            /// NOTE: already added jobs are ignored
            void scheduleJob(const ImportJobInfo& job);

            /// process next job from the list, returns false if there are no more jobs
            /// NOTE: thread safe
            bool processNextJob(IProgressTracker* progressTracker);

            //--

        private:
            IImportDepotLoader* m_loader = nullptr;
            ImportSaverThread* m_saver = nullptr;
            SourceAssetRepository* m_assets = nullptr;

            UniquePtr<ImportQueueDepotChecker> m_importerDepotChecker;
            UniquePtr<Importer> m_importer;

            //--

            struct LocalJobInfo : public NoCopy
            {
                ImportJobInfo info;
            };

            SpinLock m_jobLock;
            Array<LocalJobInfo*> m_jobsList;
            HashMap<StringBuf, LocalJobInfo*> m_jobsMap;
            Queue<LocalJobInfo*> m_jobQueue;

            const LocalJobInfo* popNextJob();

            //--

            IImportQueueCallbacks* m_callbacks = nullptr;
        };

        //--

    } // res
} // base