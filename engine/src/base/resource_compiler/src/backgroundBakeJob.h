/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: service #]
***/

#pragma once

#include "backgroundBakeService.h"

namespace base
{
    namespace res
    {
        //--

        class IBackgroundBakerNotification;
        class BakingJobList;

        //--

        class BakingJob : public IBakingJob, public IProgressTracker
        {
        public:
            BakingJob(const ResourceKey& key, bool saveResults);
            virtual ~BakingJob();

            // IBakingJob
            virtual void cancel() override final;
            virtual bool finished() const override final;
            virtual BakingJobResult finalStatus() const override final;
            virtual ResourcePtr finalData() const override final;
            virtual float finalProcessingTime() const override final;
            virtual StringBuf lastStatus() const override final;
            virtual float lastProgress() const override final;

            // IProgressTracker
            virtual bool checkCancelation() const override final;
            virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text) override final;

            //--

            bool m_saveResults;

            std::atomic<bool> m_finished = false;
            std::atomic<bool> m_canceled = false;
            std::atomic<BakingJobResult> m_finalStatus = BakingJobResult::Finished;
            std::atomic<float> m_finalProcessingTime = 0.0f;

            ResourcePtr m_finalData;

            SpinLock m_statusLock;
            StringBuf m_lastStatusText;
            float m_lastStatusProgress = -1.0f;
        };

        //--

        class BakingJobList : public NoCopy
        {
        public:
            BakingJobList(depot::DepotStructure* depot, IResourceLoader* loader);
            virtual ~BakingJobList();

            //--

            /// get current number of jobs
            uint32_t currentJobCount() const;

            /// issue a baking job
            BakingJobPtr issueJob(const ResourceKey& key, bool cancelExisting, bool saveResults);

            /// update jobs
            void update();

            //--

            /// attach listener
            void attachListener(IBackgroundBakerNotification* listener);

            /// detach listener
            void dettachListener(IBackgroundBakerNotification* listener);

            //--

        private:
            IResourceLoader* m_loader = nullptr;
            depot::DepotStructure* m_depot = nullptr;

            Array<RefPtr<BakingJob>> m_activeBakingJobs;

            Array<IBackgroundBakerNotification*> m_listeners;
            Mutex m_listenerLock;

            //--

            void handleBackgroundBakingStarted(const ResourceKey& key, float waitTime);
            void handleBackgroundBakingFinished(const ResourceKey& key, bool valid, float timeTaken);
        };

        //--

    } // res
} // base 