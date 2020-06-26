/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/resources/include/resourceLoader.h"
#include "base/resources/include/resourceLoaderCached.h"
#include "base/resources/include/resourceMetadata.h"
#include "base/containers/include/queue.h"
#include "base/containers/include/heap.h"

#include "base/system/include/thread.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/io/include/timestamp.h"

namespace base
{
    namespace cooker
    {

        //--

        class Cooker;
        class MetadataCache;
        class DependencyTracker;

        //--

        /// 

        //--

        /// a resource loader that cooks resources on demand, results are never cached
        class BASE_COOKING_API ResourceLoaderCooker : public res::IResourceLoaderCached
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceLoaderCooker, res::IResourceLoaderCached);

        public:
            ResourceLoaderCooker();
            virtual ~ResourceLoaderCooker();

            //--

            // source depot structure we cook from
            INLINE depot::DepotStructure& depot() const { return *m_depot; }

            //--

            // IResourceLoader
            virtual void update() override final;
            virtual bool initialize(const app::CommandLine& cmdLine) override final;
            virtual res::ResourceHandle loadResourceOnce(const res::ResourceKey& key) override final CAN_YIELD;
            virtual bool validateExistingResource(const res::ResourceHandle& res, const res::ResourceKey& key) const override final;
            virtual depot::DepotStructure* queryUncookedDepot() const override final;

        private:
            UniquePtr<depot::DepotStructure> m_depot;
            UniquePtr<Cooker> m_cooker; // actual workhorse

            UniquePtr<DependencyTracker> m_depTracker;

            Queue<res::ResourceKey> m_pendingReloadQueue;
            HashSet<res::ResourceKey> m_pendingReloadSet;

            res::ResourceKey m_resourceBeingReloadedKey;
            res::ResourceHandle m_resourceBeingReloaded;
            res::ResourceHandle m_reloadedResource;
            bool m_reloadFinished = false;
            SpinLock m_reloadLock;

            void checkChangedFiles();

            res::ResourcePtr pickupNextResourceForReloading_NoLock();

            void startReloading();
            void processReloading(res::ResourceKey key);

            bool tryFinishReloading(res::ResourcePtr& outReloadedResource, res::ResourcePtr& outNewResource);
            void applyReloading(res::ResourcePtr currentResource, res::ResourcePtr newResource);

            res::ResourcePtr loadInternal(res::ResourceKey key, bool normalLoading=true);
            res::ResourcePtr loadInternalDirectly(ClassType resClass, base::StringView<char> depothPath);

            void updateDirectFileDependencies(res::ResourceKey key, base::StringView<char> depothPath);
            void updateEmptyFileDependencies(res::ResourceKey key, base::StringView<char> depothPath);
        };

        //--

    } // cooker
} // base