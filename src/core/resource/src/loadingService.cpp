/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resource #]
***/

#include "build.h"
#include "loader.h"
#include "loadingService.h"

#include "core/resource/include/loader.h"
#include "core/app/include/configService.h"
#include "core/app/include/commandline.h"

BEGIN_BOOMER_NAMESPACE()

//--

ConfigProperty<uint32_t> cvFileRetentionTime("Loader", "FileRetentionTime", 30);
LoadingService* GGlobalResourceLoadingService = nullptr;

//--

RTTI_BEGIN_TYPE_CLASS(LoadingService);
RTTI_END_TYPE();

LoadingService::LoadingService()
{}

app::ServiceInitializationResult LoadingService::onInitializeService(const app::CommandLine& cmdLine)
{
    // create and initialize the loader
    m_resourceLoader = RefNew<ResourceLoader>();

    // initialized
    GGlobalResourceLoadingService = this;
    return app::ServiceInitializationResult::Finished;
}

void LoadingService::onShutdownService()
{
    // detach global loader
    GGlobalResourceLoadingService = nullptr;

    // detach loader
    m_resourceLoader.reset();
}

void LoadingService::onSyncUpdate()
{
    releaseRetainedFiles();
    m_resourceLoader->processReloadEvents();
}

void LoadingService::addRetainedFile(const ResourcePtr& file)
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

ResourcePtr LoadingService::loadResource(const ResourcePath& path)
{
    auto ret = m_resourceLoader->loadResource(path);
    addRetainedFile(ret);
    return ret;
}

//--

void LoadingService::loadResourceAsync(const ResourcePath& key, const std::function<void(const ResourcePtr&)>& funcLoaded)
{
    // ask the resource loader if it already has the resource
    {
        ResourcePtr existingResource;
        if (m_resourceLoader->acquireLoadedResource(key, existingResource))
        {
            if (existingResource)
            {
                funcLoaded(existingResource);
                return;
            }
        }
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
                    WaitForFence(validLoadingJob->m_signal);
                    funcLoaded(validLoadingJob->m_loadedResource);
                };

                return;
            }
        }
    }

    // start new async loading job
    auto newAsyncLoadingJob = RefNew<AsyncLoadingJob>();
    newAsyncLoadingJob->m_signal = CreateFence("AsyncResourceLoadSignal");
    m_asyncLoadingJobsMap[key] = newAsyncLoadingJob;

    // notify resource was queued
    //m_depotFileStatusMonitor->notifyFileStatusChanged(ResourceKey(path, resClass), FileState::Queued);

    // start loading, NOTE: in the background
    RunFiber("LoadResourceAsync") << [this, key, newAsyncLoadingJob, funcLoaded](FIBER_FUNC)
    {
        auto loadedResource = loadResource(key);
        funcLoaded(loadedResource);

        newAsyncLoadingJob->m_loadedResource = loadedResource;
        SignalFence(newAsyncLoadingJob->m_signal);
    };
}

//--

bool LoadingService::acquireLoadedResource(const ResourcePath& key, ResourcePtr& outLoadedPtr)
{
    return m_resourceLoader->acquireLoadedResource(key, outLoadedPtr);
}

END_BOOMER_NAMESPACE()

//--

BEGIN_BOOMER_NAMESPACE()

//--

ResourceLoader* GlobalLoader()
{
    if (GGlobalResourceLoadingService)
        return GGlobalResourceLoadingService->loader();

    return nullptr;
}

ResourcePtr LoadResource(const ResourcePath& path)
{
    if (GGlobalResourceLoadingService)
        return GGlobalResourceLoadingService->loadResource(path);

    TRACE_ERROR("No global resource loader specified, loading of resoruce '{}' will fail", path);
    return nullptr;
}

void LoadResourceAsync(const ResourcePath& path, const std::function<void(const ResourcePtr&)>& funcLoaded)
{
    if (GGlobalResourceLoadingService)
    {
        GGlobalResourceLoadingService->loadResourceAsync(path, funcLoaded);
    }
    else
    {
        TRACE_ERROR("No global resource loader specified, loading of resoruce '{}' will fail", path);
        funcLoaded(nullptr);
    }
}

//--

END_BOOMER_NAMESPACE()
