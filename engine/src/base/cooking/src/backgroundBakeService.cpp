/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#include "build.h"
#include "backgroundBakeJob.h"
#include "backgroundBakeService.h"
#include "backgroundSaveService.h"

#include "cooker.h"
#include "cookerResourceLoader.h"

#include "base/depot/include/depotStructure.h"
#include "base/resources/include/resourceLoadingService.h"
#include "base/app/include/commandline.h"

namespace base
{
    namespace cooker
    {
        //--

        ConfigProperty<bool> cvBackgroundBakingEnabled("Resource.Baker", "EnableBackgroundBake", true);
        ConfigProperty<int> cvBackgroundSimultaneousJobs("Resource.Baker", "NumSimultaneousJobs", 1);

        //--

        IBackgroundBakerNotification::~IBackgroundBakerNotification()
        {}

        //--

        IBakingJob::IBakingJob(const res::ResourceKey& key)
            : m_key(key)
        {}

        IBakingJob::~IBakingJob()
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(BackgroundBaker);
            RTTI_METADATA(app::DependsOnServiceMetadata).dependsOn<base::res::LoadingService>();
        RTTI_END_TYPE();

        //--

        BackgroundBaker::BackgroundBaker()
        {}

        BackgroundBaker::~BackgroundBaker()
        {}

        bool BackgroundBaker::enabled() const
        {
            return cvBackgroundBakingEnabled.get();
        }

        void BackgroundBaker::enabled(bool flag)
        {
            cvBackgroundBakingEnabled.set(flag);
        }

        uint32_t BackgroundBaker::maxJobCount() const
        {
            return cvBackgroundSimultaneousJobs.get();
        }

        void BackgroundBaker::maxJobCount(uint32_t count)
        {
            cvBackgroundSimultaneousJobs.set(count);
        }

        void BackgroundBaker::attachListener(IBackgroundBakerNotification* listener)
        {
            if (listener)
            {
                auto lock = CreateLock(m_listenerLock);
                m_listeners.pushBackUnique(listener);

                m_bakingJobList->attachListener(listener);
            }
        }

        void BackgroundBaker::dettachListener(IBackgroundBakerNotification* listener)
        {
            auto lock = CreateLock(m_listenerLock);
            auto index = m_listeners.find(listener);
            if (index != INDEX_NONE)
                m_listeners[index] = nullptr;

            m_bakingJobList->dettachListener(listener);
        }

        void BackgroundBaker::handleBackgroundBakingRequested(const res::ResourceKey& key, bool forced)
        {
            auto lock = CreateLock(m_listenerLock);

            for (auto* listener : m_listeners)
                if (listener)
                    listener->handleBackgroundBakingRequested(key, forced);

            m_listeners.remove(nullptr);
        }

        void BackgroundBaker::handleBackgroundBakingMissingBakedResource(const res::ResourceKey& key)
        {
            auto lock = CreateLock(m_listenerLock);

            for (auto* listener : m_listeners)
                if (listener)
                    listener->handleBackgroundBakingMissingBakedResource(key);

            m_listeners.remove(nullptr);
        }

        app::ServiceInitializationResult BackgroundBaker::onInitializeService(const app::CommandLine& cmdLine)
        {
            // only use in editor
            if (!cmdLine.hasParam("editor"))
                return app::ServiceInitializationResult::Silenced;

            // we can initialize the managed depot only if we are running from uncooked data
            auto loaderService = GetService<res::LoadingService>();
            if (!loaderService)
            {
                TRACE_ERROR("No resource loading service spawned, no editor can be created");
                return app::ServiceInitializationResult::Silenced;
            }

            // we need a cooker based loader to support resource edition
            auto loader = rtti_cast<cooker::ResourceLoaderCooker>(loaderService->loader());
            if (!loader)
            {
                TRACE_ERROR("Resource loader does not support cooking from depot, no editor can be created");
                return app::ServiceInitializationResult::Silenced;
            }

            // get the depot
            m_loader = loader;
            m_depot = &loader->depot();
            m_loader->attachListener(this);

            // create baker
            m_bakingJobList = MemNew(BakingJobList, m_depot, m_loader);

            // TODO: load previous list of files (to provide session persistence)

            return app::ServiceInitializationResult::Finished;
        }

        void BackgroundBaker::onShutdownService()
        {
            auto lock = base::CreateLock(m_jobsLock);

            MemDelete(m_bakingJobList);
            m_bakingJobList = nullptr;

            if (m_loader)
                m_loader->dettachListener(this);
        }

        BakingJobPtr BackgroundBaker::bake(const res::ResourceKey& key, bool cancelExisting /*= true*/, bool saveResult /*= true*/)
        {
            auto lock = base::CreateLock(m_jobsLock);
            handleBackgroundBakingRequested(key, true);
            return m_bakingJobList->issueJob(key, cancelExisting, saveResult);
        }

        void BackgroundBaker::onSyncUpdate()
        {
            auto lock = base::CreateLock(m_jobsLock);

            // update job list
            m_bakingJobList->update();
           
            // push pending jobs to the list
            {
                auto lock2 = base::CreateLock(m_newJobsLock);

                auto newJobs = std::move(m_newJobs);
                m_newJobsSet.reset();
                m_newJobs.reset();

                for (const auto& info : newJobs)
                {
                    // find existing job
                    PendingJob* job = nullptr;
                    if (!m_jobMap.find(info.key, job))
                    {
                        job = MemNew(PendingJob);
                        job->key = info.key;
                        job->scheduledTime = info.scheduledTime;
                        m_pendingJobs.pushBack(job);
                        m_jobMap[info.key] = job;

                        handleBackgroundBakingRequested(info.key, false);
                    }
                }
            }

            // start some new cooking
            if (cvBackgroundBakingEnabled.get())
            {
                auto currentRunningJobs = m_bakingJobList->currentJobCount();
                auto maxJobsToStart = std::max<int>(0, cvBackgroundSimultaneousJobs.get() - (int)currentRunningJobs);
                maxJobsToStart = std::min<int>(maxJobsToStart, (int)m_pendingJobs.size());
                if (maxJobsToStart > 0)
                {
                    TRACE_INFO("Starting {} background backing jobs", maxJobsToStart);
                    for (int i = 0; i < maxJobsToStart; ++i)
                    {
                        auto* job = m_pendingJobs[i];

                        m_bakingJobList->issueJob(job->key, false, true);

                        m_jobMap.remove(job->key);
                        MemDelete(job);
                    }

                    m_pendingJobs.erase(0, maxJobsToStart);
                }
            }

        }

        void BackgroundBaker::onResourceMissingBakedResource(const res::ResourceKey& key)
        {
            handleBackgroundBakingMissingBakedResource(key);
            scheduleResourceForBackgroundBaking(key);
        }

        void BackgroundBaker::scheduleResourceForBackgroundBaking(const res::ResourceKey& key)
        {
            auto lock = base::CreateLock(m_newJobsLock);

            if (m_newJobsSet.insert(key))
            {
                auto& entry = m_newJobs.emplaceBack();
                entry.key = key;
            }
        }

        //--

    } // depot
} // base
