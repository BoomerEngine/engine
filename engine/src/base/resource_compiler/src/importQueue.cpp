/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importer.h"
#include "importQueue.h"
#include "base/resource/include/resourceMetadata.h"
#include "importSaveThread.h"

namespace base
{
    namespace res
    {
        //--

        IImportQueueCallbacks::~IImportQueueCallbacks()
        {}

        //--

        IImportDepotLoader::~IImportDepotLoader()
        {}

        //--

        class ImportQueueNullCallbacks : public IImportQueueCallbacks
        {
        public:
            ImportQueueNullCallbacks()
            {}
        };

        //--

        ImportQueue::ImportQueue(SourceAssetRepository* assets, IImportDepotLoader* loader, ImportSaverThread* saver, IImportQueueCallbacks* callbacks)
            : m_assets(assets)
            , m_loader(loader)
            , m_saver(saver)
            , m_callbacks(callbacks)
        {
            m_importer.create(assets);

            if (!m_callbacks)
            {
                static ImportQueueNullCallbacks theNullCallbacks;
                m_callbacks = &theNullCallbacks;
            }
        }

        ImportQueue::~ImportQueue()
        {}

        void ImportQueue::signalCancelRequest()
        {
            if (0 == m_asyncCancelationFlag.exchange(1))
            {
                TRACE_INFO("Requested job cancelation");
            }
        }

        void ImportQueue::scheduleJob(const ImportJobInfo& job)
        {
            if (job.depotFilePath && job.assetFilePath)
            {
                const auto key = job.depotFilePath.toLower();

                auto lock = CreateLock(m_jobLock);
                if (!m_jobsMap.contains(key))
                {
                    auto* jobInfo = MemNew(LocalJobInfo).ptr;
                    jobInfo->info = job;
                    m_jobsList.pushBack(jobInfo);
                    m_jobQueue.push(jobInfo);
                    m_jobsMap[key] = jobInfo;

                    m_callbacks->queueJobAdded(job);
                }
            }
        }

        const ImportQueue::LocalJobInfo* ImportQueue::popNextJob()
        {
            auto lock = CreateLock(m_jobLock);
            if (m_jobQueue.empty())
                return nullptr;

            auto* job = m_jobQueue.top();
            m_jobQueue.pop();
            return job;
        }

        class ImportQueueProgressTracker : public IProgressTracker
        {
        public:
            ImportQueueProgressTracker(IImportQueueCallbacks* callbacks, const StringBuf& depotPath, std::atomic<uint32_t>* cancelationFlag)
                : m_depotPath(depotPath)
                , m_callbacks(callbacks)
                , m_cancelationFlag(cancelationFlag)
            {}

            virtual bool checkCancelation() const override final
            {
                return m_cancelationFlag->load();
            }

            virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text) override final
            {
                m_callbacks->queueJobProgressUpdate(m_depotPath, currentCount, totalCount, text);
            }

        private:
            StringBuf m_depotPath;

            IImportQueueCallbacks* m_callbacks = nullptr;
            std::atomic<uint32_t>* m_cancelationFlag = nullptr;
        };

        bool ImportQueue::processNextJob()
        {
            // get next job to process
            const auto* job = popNextJob();
            if (!job)
                return false;

            // notify callbacks
            ScopeTimer timer;
            m_callbacks->queueJobStarted(job->info.depotFilePath);

            // check if resource is up to date
            if (m_loader)
            {
                if (const auto currentMetadata = m_loader->loadExistingMetadata(job->info.depotFilePath))
                {
                    const auto status = m_importer->checkStatus(*currentMetadata);
                    if (status == ImportStatus::UpToDate)
                    {
                        m_callbacks->queueJobFinished(job->info.depotFilePath, ImportStatus::UpToDate, timer.timeElapsed());

                        // follow up with resources that were imported last time
                        for (const auto& followup : currentMetadata->importFollowups)
                        {
                            ImportJobInfo jobInfo;
                            jobInfo.depotFilePath = followup.depotPath;
                            jobInfo.assetFilePath = followup.sourceImportPath;
                            jobInfo.externalConfig = followup.configuration;
                            jobInfo.userConfig = nullptr; // no specific user configuration, will use the resource's one
                            scheduleJob(jobInfo);
                        }

                        return true;
                    }
                }
            }

            // we are going to import, load existing data as an aid
            ResourcePtr existingResource;
            if (m_loader)
                existingResource = m_loader->loadExistingResource(job->info.depotFilePath);

            // import resource
            ResourcePtr importedResource;
            {
                ImportQueueProgressTracker localProgressTracker(m_callbacks, job->info.depotFilePath, &m_asyncCancelationFlag);
                const auto ret = m_importer->importResource(job->info, existingResource, importedResource, &localProgressTracker);
                m_callbacks->queueJobFinished(job->info.depotFilePath, ret, timer.timeElapsed());
            }

            // if resource was imported send it to saving
            if (importedResource && m_saver)
                m_saver->scheduleSave(importedResource, job->info.depotFilePath);

            // we are done
            return true;
        }

        //--

    } // res
} // base
