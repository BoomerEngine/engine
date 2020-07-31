/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resource #]
***/

#pragma once

#include "base/app/include/localService.h"
#include "base/containers/include/queue.h"
#include "resourceReference.h"

namespace base
{
    namespace res
    {
        //---

        // resource loading service
        class BASE_RESOURCE_API LoadingService : public app::ILocalService
        {
            RTTI_DECLARE_VIRTUAL_CLASS(LoadingService, app::ILocalService);

        public:
            LoadingService();
            
            //--

            /// created resource loader
            INLINE const RefPtr<IResourceLoader>& loader() const { return m_resourceLoader; }

            //--

            /// load resource from specified path and of specified class (can be subclass)
            /// NOTE: this will yield the current job until the resource is loaded
            CAN_YIELD BaseReference loadResource(const ResourceKey& key);

            /// nice helper for async loading of resources if the resource exists it's returned right away without any extra fibers created (it's the major performance win)
            /// if resource does not exist it's queued for loading and internal fiber is created to service it
            /// NOTE: if the resource exists at the moment of the call the callback function is called right away
            void loadResourceAsync(const ResourceKey& key, const std::function<void(const BaseReference&)>& funcLoaded);

            //--

            /// check if resource is in use
            bool acquireLoadedResource(const ResourceKey& key, ResourcePtr& outLoadedPtr);

        protected:
            virtual app::ServiceInitializationResult onInitializeService( const app::CommandLine& cmdLine) override final;
            virtual void onShutdownService() override final;
            virtual void onSyncUpdate() override final;

            RefPtr<IResourceLoader> m_resourceLoader;

            //---

            struct AsyncLoadingJob : public IReferencable
            {
                BaseReference m_loadedResource;
                fibers::WaitCounter m_signal;
            };

            HashMap<ResourceKey, RefWeakPtr<AsyncLoadingJob>> m_asyncLoadingJobsMap;
            SpinLock m_asyncLoadingJobsMapLock;

            //---

            struct RetainedFile
            {
                NativeTimePoint m_expiration;
                ResourceHandle m_ptr;
            };

            Queue<RetainedFile> m_retainedFiles;
            SpinLock m_retainedFilesLock;

            void addRetainedFile(const ResourceHandle& file);
            void releaseRetainedFiles();

            //--
        };

        //---

    } // res
} // base
