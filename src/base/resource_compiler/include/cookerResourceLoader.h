/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "base/resource/include/resourceLoader.h"
#include "base/resource/include/resourceLoaderCached.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/containers/include/queue.h"
#include "base/containers/include/heap.h"

#include "base/system/include/thread.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/io/include/timestamp.h"

namespace base
{
    namespace res
    {

        //--

        class Cooker;
        class MetadataCache;
        class DependencyTracker;

        //--

        /// 

        //--

        /// a resource loader that cooks resources on demand, results are never cached
        class BASE_RESOURCE_COMPILER_API ResourceLoaderCooker : public IResourceLoaderCached
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceLoaderCooker, IResourceLoaderCached);

        public:
            ResourceLoaderCooker();
            virtual ~ResourceLoaderCooker();

            //--

            // source depot structure we cook from
            INLINE depot::DepotStructure& depot() const { return *m_depot; }

        protected:

            // IResourceLoader
            virtual void update() override final;
            virtual bool initialize(const app::CommandLine& cmdLine) override final;
            virtual ResourceHandle loadResourceOnce(const ResourceKey& key) override final CAN_YIELD;
            virtual bool validateExistingResource(const ResourceHandle& res, const ResourceKey& key) const override final;
            virtual depot::DepotStructure* queryUncookedDepot() const override final;
            virtual void notifyResourceReloaded(const ResourceHandle& currentResource, const ResourceHandle& newResource) override final;

        private:
            UniquePtr<depot::DepotStructure> m_depot;
            UniquePtr<Cooker> m_cooker; // actual workhorse

            //--

            UniquePtr<DependencyTracker> m_depTracker;

            //--

            struct PendingReload
            {
                ResourcePtr currentResource;
                ResourcePtr newResource;
            };

            SpinLock m_pendingReloadQueueLock;
            Queue<PendingReload> m_pendingReloadQueue;
            
            //--

            void checkChangedFiles();

            bool popNextReload(PendingReload& outReload);
            void applyReload(ResourcePtr currentResource, ResourcePtr newResource);

            virtual ResourceKey translateResourceKey(const ResourceKey& key) const override;
        };

        //--

    } // res
} // base