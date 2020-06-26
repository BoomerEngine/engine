/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#include "build.h"
#include "backgroundBakeJob.h"
#include "backgroundSaveService.h"

#include "cooker.h"
#include "cookerResourceLoader.h"

#include "base/depot/include/depotStructure.h"
#include "base/resources/include/resourceLoadingService.h"

namespace base
{
    namespace cooker
    {
        //--

        BakingJob::BakingJob(const res::ResourceKey& key, bool saveResults)
            : IBakingJob(key)
            , m_saveResults(saveResults)
        {}

        BakingJob::~BakingJob()
        {}

        void BakingJob::cancel()
        {
            if (!m_canceled.exchange(true))
            {
                TRACE_WARNING("Current baking job for '{}' has been canceled", key());
            }
        }

        bool BakingJob::finished() const
        {
            return m_finished.load();
        }

        BakingJobResult BakingJob::finalStatus() const
        {
            return m_finalStatus.load();
        }

        res::ResourcePtr BakingJob::finalData() const
        {
            if (m_finished)
                return m_finalData;
            else
                return nullptr;
        }

        float BakingJob::finalProcessingTime() const
        {
            if (m_finished)
                return m_finalProcessingTime.load();
            else
                return 0.0f;
        }

        StringBuf BakingJob::lastStatus() const
        {
            auto lock = CreateLock(m_statusLock);
            return m_lastStatusText;
        }

        float BakingJob::lastProgress() const
        {
            auto lock = CreateLock(m_statusLock);
            return m_lastStatusProgress;
        }

        bool BakingJob::checkCancelation() const
        {
            return m_canceled.load();
        }

        void BakingJob::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text)
        {
            auto lock = CreateLock(m_statusLock);
            m_lastStatusProgress = totalCount ? ((double)currentCount / (double)totalCount) : -1.0f;
            m_lastStatusText = base::StringBuf(text);
        }

        //--

        BakingJobList::BakingJobList(depot::DepotStructure* depot, res::IResourceLoader* loader)
            : m_depot(depot)
            , m_loader(loader)
        {}

        BakingJobList::~BakingJobList()
        {
            if (!m_activeBakingJobs.empty())
            {
                TRACE_INFO("There are still '{}' baking jobs, canceling them", m_activeBakingJobs.size());

                for (const auto& job : m_activeBakingJobs)
                    job->cancel();

                for (;;)
                {
                    bool hasSomeRunningJobs = false;

                    uint32_t numUnfinishedJobs = 0;
                    for (const auto& job : m_activeBakingJobs)
                        if (!job->finished())
                            numUnfinishedJobs += 1;

                    if (numUnfinishedJobs)
                    {
                        TRACE_INFO("Waiting for remaining {} backing job(s) to finish...", numUnfinishedJobs);
                        Sleep(500);
                    }
                }
            }
        }

        void BakingJobList::attachListener(IBackgroundBakerNotification* listener)
        {
            if (listener)
            {
                auto lock = CreateLock(m_listenerLock);
                m_listeners.pushBackUnique(listener);
            }

        }

        void BakingJobList::dettachListener(IBackgroundBakerNotification* listener)
        {
            auto lock = CreateLock(m_listenerLock);
            auto index = m_listeners.find(listener);
            if (index != INDEX_NONE)
                m_listeners[index] = nullptr;
        }
        
        void BakingJobList::handleBackgroundBakingStarted(const res::ResourceKey& key, float waitTime)
        {
            auto lock = CreateLock(m_listenerLock);
            for (auto* listener : m_listeners)
                if (listener)
                    listener->handleBackgroundBakingStarted(key, waitTime);

            m_listeners.remove(nullptr);
        }

        void BakingJobList::handleBackgroundBakingFinished(const res::ResourceKey& key, bool valid, float timeTaken)
        {
            auto lock = CreateLock(m_listenerLock);
            for (auto* listener : m_listeners)
                if (listener)
                    listener->handleBackgroundBakingFinished(key, valid, timeTaken);

            m_listeners.remove(nullptr);
        }

        uint32_t BakingJobList::currentJobCount() const
        {
            return m_activeBakingJobs.size();
        }

        BakingJobPtr BakingJobList::issueJob(const res::ResourceKey& key, bool cancelExisting, bool saveResults)
        {
            if (!key)
                return false;

            for (const auto& job : m_activeBakingJobs)
            {
                if (job->key() == key)
                {
                    if (!cancelExisting)
                        return job;
                    job->cancel();
                }
            }

            auto job = base::CreateSharedPtr<BakingJob>(key, saveResults);
            m_activeBakingJobs.pushBack(job);

            // process the job
            RunFiber("BackgroundBaking") << [this, job](FIBER_FUNC)
            {
                if (job->m_canceled)
                {
                    TRACE_INFO("Background baking of '{}' was canceled before it started", job->key());
                    job->m_finalStatus = BakingJobResult::Canceled;
                    job->m_finished = true;

                    handleBackgroundBakingFinished(job->key(), false, 0.0f);
                }
                else
                {
                    TRACE_INFO("Started background baking of '{}'", job->key());

                    handleBackgroundBakingStarted(job->key(), 0.0f);

                    {
                        ScopeTimer timer;
                        Cooker cooker(*m_depot, m_loader, job);

                        auto data = cooker.cook(job->key());

                        if (job->m_canceled)
                        {
                            job->m_finalStatus = BakingJobResult::Canceled;
                            TRACE_INFO("Failed background baking of '{}' because job was canceled", job->key());
                        }
                        else if (data)
                        {
                            job->m_finalStatus = BakingJobResult::Finished;
                            job->m_finalData = data;
                            job->m_finalProcessingTime = timer.timeElapsed();
                            TRACE_INFO("Finished background baking of '{}' in {}", job->key(), job->m_finalProcessingTime.load());
                        }
                        else
                        {
                            job->m_finalStatus = BakingJobResult::Failed;
                            TRACE_INFO("Failed background baking of '{}'", job->key());
                        }

                        job->m_finished = true;
                    }

                    if (job->m_finalStatus.load() == BakingJobResult::Finished)
                        handleBackgroundBakingFinished(job->key(), true, job->m_finalProcessingTime.load());
                    else if (job->m_finalStatus.load() == BakingJobResult::Failed)
                        handleBackgroundBakingFinished(job->key(), false, 0.0f);
                }
            };

            return job;
        }

        void BakingJobList::update()
        {
            auto processingJobs = std::move(m_activeBakingJobs);
            for (const auto& job : processingJobs)
            {
                if (job->m_finished.load())
                {
                    if (job->m_finalStatus.load() == BakingJobResult::Finished)
                    {
                        if (!job->m_canceled.load() && job->m_saveResults)
                        {
                            if (auto saveService = GetService<BackgroundSaver>())
                            {
                                const auto saveExtension = res::IResource::GetResourceExtensionForClass(job->m_finalData->cls());
                                DEBUG_CHECK_EX(saveExtension, "Baked file should have some kind of extension");
                                if (saveExtension)
                                {
                                    base::StringBuilder saveDepotPath;
                                    //saveDepotPath << job->key().path();
                                    saveDepotPath << job->key().path().directory();
                                    saveDepotPath << ".boomer/";
                                    saveDepotPath << job->key().path().fileName();
                                    saveDepotPath << "." << saveExtension;

                                    saveService->scheduleSave(saveDepotPath.toString(), job->m_finalData);
                                }
                            }
                        }
                    }
                }
                else
                {
                    // still processing
                    m_activeBakingJobs.pushBack(job);
                }
            }
        }

        //--

    } // depot
} // base
