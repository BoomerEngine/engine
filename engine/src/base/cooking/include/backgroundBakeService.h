/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/app/include/localService.h"
#include "base/socket/include/tcpServer.h"
#include "base/resources/include/resourceLoader.h"

namespace base
{
    namespace cooker
    {
        //--
        
        class BakingJobList;

        //--

        /// callback interface for the background baker
        class BASE_COOKING_API IBackgroundBakerNotification : public NoCopy
        {
        public:
            virtual ~IBackgroundBakerNotification();

            /// create a background baking request for given resource
            virtual void handleBackgroundBakingMissingBakedResource(const res::ResourceKey& key) = 0;

            /// create a background baking request for given resource
            virtual void handleBackgroundBakingRequested(const res::ResourceKey& key, bool forced) = 0;

            /// background backing for given resource has started
            virtual void handleBackgroundBakingStarted(const res::ResourceKey& key, float waitTime) = 0;

            /// background backing for given resource has finished
            virtual void handleBackgroundBakingFinished(const res::ResourceKey& key, bool valid, float timeTaken) = 0;
        };

        //--

        /// result of baking job
        enum class BakingJobResult : uint8_t
        {
            Finished,
            Canceled,
            Failed,
        };

        /// create a forced bake request
        class BASE_COOKING_API IBakingJob : public IReferencable
        {
        public:
            IBakingJob(const res::ResourceKey& key);
            virtual ~IBakingJob();

            //--

            INLINE const res::ResourceKey& key() const { return m_key; }

            //--

            // cancel job - stops it as soon as possible and prevents saving
            // NOTE: takes no effect if file was already saved
            virtual void cancel() = 0;

            // did the job finished ?
            virtual bool finished() const = 0;

            // if the job finished, get the final result - was is successful? did it fail? etc
            virtual BakingJobResult finalStatus() const = 0;

            // get the baked data (valid only if job finished without errors)
            virtual res::ResourcePtr finalData() const = 0;

            // get the total processing time (for stats) - valid only if job finished without errors
            virtual float finalProcessingTime() const = 0;

            //--

            // get last status that was posted from the cooker (display only)
            virtual StringBuf lastStatus() const = 0;

            // get last progress that was posted from the cooker (display only)
            virtual float lastProgress() const = 0;

        private:
            res::ResourceKey m_key;
        };

        //--

        /// background resource baking service
        class BASE_COOKING_API BackgroundBaker : public app::ILocalService, public res::IResourceLoaderEventListener
        {
            RTTI_DECLARE_VIRTUAL_CLASS(BackgroundBaker, app::ILocalService);

        public:
            BackgroundBaker();
            virtual ~BackgroundBaker();

            //--

            bool enabled() const;
            void enabled(bool flag);

            uint32_t maxJobCount() const;
            void maxJobCount(uint32_t count);

            //--

            /// attach listener
            void attachListener(IBackgroundBakerNotification* listener);

            /// detach listener
            void dettachListener(IBackgroundBakerNotification* listener);

            //--

            /// add resource to background backing list
            /// NOTE: fails if we have a forced bake request
            void scheduleResourceForBackgroundBaking(const res::ResourceKey& key);

            //--

            /// create a forced baking request
            BakingJobPtr bake(const res::ResourceKey& key, bool cancelExisting=true, bool saveResult=true);

        private:
            res::IResourceLoader* m_loader = nullptr;
            depot::DepotStructure* m_depot = nullptr;

            struct PendingJob : public NoCopy
            {
                res::ResourceKey key;
                std::atomic<bool> canceled = false;
                NativeTimePoint scheduledTime = NativeTimePoint::Now();
            };

            Array<PendingJob*> m_pendingJobs;
            HashMap<res::ResourceKey, PendingJob*> m_jobMap;
            Mutex m_jobsLock;

            struct NewJob
            {
                res::ResourceKey key;
                NativeTimePoint scheduledTime = NativeTimePoint::Now();
            };

            Array<NewJob> m_newJobs;
            HashSet<res::ResourceKey> m_newJobsSet;
            SpinLock m_newJobsLock;

            //--

            BakingJobList* m_bakingJobList = nullptr;

            //--

            Array<IBackgroundBakerNotification*> m_listeners;
            Mutex m_listenerLock;

            void handleBackgroundBakingMissingBakedResource(const res::ResourceKey& key);
            void handleBackgroundBakingRequested(const res::ResourceKey& key, bool forced);

            //--

            virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
            virtual void onShutdownService() override final;
            virtual void onSyncUpdate() override final;

            // IResourceLoaderEventListener
            virtual void onResourceMissingBakedResource(const res::ResourceKey& path) override final;
        };

        //--

    } // cooker
} // base 