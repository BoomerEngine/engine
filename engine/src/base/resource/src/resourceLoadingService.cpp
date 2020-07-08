/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: services #]
***/

#include "build.h"
#include "resourceLoader.h"
#include "resourceLoadingService.h"

#include "base/resource/include/resourceLoader.h"
#include "base/app/include/configService.h"
#include "base/app/include/commandline.h"

namespace base
{
    namespace res
    {

        //--

        ConfigProperty<uint32_t> cvFileRetentionTime("Loader", "FileRetentionTime", 30);
        ConfigProperty<StringBuf> cvDefaultResourceLoaderClass("Loader", "DefaultLoaderClass", "base::res::ResourceLoaderCooker");
        ConfigProperty<StringBuf> cvFinalResourceLoaderClass("Loader", "FinalLoaderClass", "base::res::ResourceLoaderFinal");
        LoadingService* GGlobalResourceLoadingService = nullptr;

        //--

        RTTI_BEGIN_TYPE_CLASS(LoadingService);
            RTTI_METADATA(app::DependsOnServiceMetadata).dependsOn<config::ConfigService>();
        RTTI_END_TYPE();

        LoadingService::LoadingService()
        {}

        static StringBuf GetLoaderClass()
        {
#ifdef BUILD_FINAL
            return cvFinalResourceLoaderClass.get();
#else
            return cvDefaultResourceLoaderClass.get();
#endif
        }

        app::ServiceInitializationResult LoadingService::onInitializeService(const app::CommandLine& cmdLine)
        {
            // find loader class
            const auto loaderClassName = GetLoaderClass();
            const auto loaderClass = RTTI::GetInstance().findClass(StringID(loaderClassName.view())).cast<IResourceLoader>();
            if (!loaderClass)
            {
                TRACE_ERROR("Resource loader class '{}' not found", loaderClassName);
                return app::ServiceInitializationResult::FatalError;
            }

            // create and initialize the loader
            auto loader = loaderClass->create<IResourceLoader>();
            if (!loader->initialize(cmdLine))
            {
                TRACE_ERROR("Failed to initialize resource loader '{}'", loaderClass->name());
                return app::ServiceInitializationResult::FatalError;
            }
            else
            {
                m_resourceLoader = loader;
            }

            // bind the depot service as the loader for static resources
            base::res::IStaticResource::BindGlobalLoader(m_resourceLoader.get());

            // initialized
            GGlobalResourceLoadingService = this;
            return app::ServiceInitializationResult::Finished;
        }

        void LoadingService::onShutdownService()
        {
            // detach global loader
            GGlobalResourceLoadingService = nullptr;

            // unbind the depot service as the loader for static resources
            base::res::IStaticResource::BindGlobalLoader(nullptr);

            // detach loader
            m_resourceLoader.reset();
        }

        void LoadingService::onSyncUpdate()
        {
            releaseRetainedFiles();

            m_resourceLoader->update();
        }

        void LoadingService::addRetainedFile(const ResourceHandle& file)
        {
            if (file)
            {
                auto lock = CreateLock(m_retainedFilesLock);

                RetainedFile entry;
                entry.m_ptr = file;
                entry.m_expiration = NativeTimePoint::Now() + (double)cvFileRetentionTime.get();
                m_retainedFiles.push(entry);
            }

        }

        void LoadingService::releaseRetainedFiles()
        {
            auto lock = CreateLock(m_retainedFilesLock);
            while (!m_retainedFiles.empty())
            {
                if (!m_retainedFiles.top().m_expiration.reached())
                    break;
                m_retainedFiles.pop();
            }
        }

        //--

        void LoadingService::attachListener(IResourceLoaderEventListener* listener)
        {
            m_resourceLoader->attachListener(listener);
        }

        void LoadingService::dettachListener(IResourceLoaderEventListener* listener)
        {
            m_resourceLoader->dettachListener(listener);
        }

        //--

        res::BaseReference LoadingService::loadResource(const ResourceKey& key)
        {
            auto ret = m_resourceLoader->loadResource(key);
            addRetainedFile(ret);
            return ret;
        }

        //--

        void LoadingService::loadResourceAsync(const ResourceKey& key, const std::function<void(const res::BaseReference&)>& funcLoaded)
        {
            // ask the resource loader if it already has the resource
            auto existingResource = m_resourceLoader->acquireLoadedResource(key);
            if (existingResource)
            {
                funcLoaded(existingResource);
                return;
            }

            // take the lock
            auto lock = CreateLock(m_asyncLoadingJobsMapLock);

            // get the loading job
            {
                RefWeakPtr<AsyncLoadingJob> asyncLoadingJob;
                if (m_asyncLoadingJobsMap.find(key, asyncLoadingJob))
                {
                    // is it still active ?
                    auto validLoadingJob = asyncLoadingJob.lock();
                    if (validLoadingJob)
                    {
                        // wait until the job finishes
                        RunChildFiber("WaitForAsyncResourceLoad") << [validLoadingJob, funcLoaded](FIBER_FUNC)
                        {
                            Fibers::GetInstance().waitForCounterAndRelease(validLoadingJob->m_signal);
                            funcLoaded(validLoadingJob->m_loadedResource);
                        };

                        return;
                    }
                }
            }

            // start new async loading job
            auto newAsyncLoadingJob = base::CreateSharedPtr<AsyncLoadingJob>();
            newAsyncLoadingJob->m_signal = Fibers::GetInstance().createCounter("AsyncResourceLoadSignal");
            m_asyncLoadingJobsMap[key] = newAsyncLoadingJob;

            // notify resource was queued
            //m_depotFileStatusMonitor->notifyFileStatusChanged(res::ResourceKey(path, resClass), FileState::Queued);

            // start loading, NOTE: in the background
            RunFiber("LoadResourceAsync") << [this, key, newAsyncLoadingJob, funcLoaded](FIBER_FUNC)
            {
                auto loadedResource = loadResource(key);
                funcLoaded(loadedResource);

                newAsyncLoadingJob->m_loadedResource = loadedResource;
                Fibers::GetInstance().signalCounter(newAsyncLoadingJob->m_signal);
            };
        }

        //--

    } // res

    //--

    res::BaseReference LoadResource(const res::ResourceKey& key)
    {
        if (res::GGlobalResourceLoadingService)
            return res::GGlobalResourceLoadingService->loadResource(key);

        TRACE_ERROR("No global resource loader specified, loading of resoruce '{}' will fail", key);
        return nullptr;
    }

    void LoadResourceAsync(const res::ResourceKey& key, const std::function<void(const res::BaseReference&)>& funcLoaded)
    {
        if (res::GGlobalResourceLoadingService)
        {
            res::GGlobalResourceLoadingService->loadResourceAsync(key, funcLoaded);
        }
        else
        {
            TRACE_ERROR("No global resource loader specified, loading of resoruce '{}' will fail", key);
            funcLoaded(nullptr);
        }
    }

    //--

} // base
