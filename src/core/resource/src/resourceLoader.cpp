/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#include "build.h"

#include "resource.h"
#include "resourceLoader.h"
#include "depotService.h"

#include "core/system/include/scopeLock.h"
#include "resourceFileLoader.h"
#include "core/object/include/objectGlobalRegistry.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

//--

#pragma optimize("", off)

ResourceLoader::ResourceLoader()
{
    m_eventKey = MakeUniqueEventKey("ResourceLoader");
}

ResourceLoader::~ResourceLoader()
{
}

//--

bool ResourceLoader::validateResource(const ResourcePath& key, const io::TimeStamp& existingTimestamp)
{
    io::TimeStamp currentFileTime;
    if (GetService<DepotService>()->queryFileTimestamp(key.view(), currentFileTime))
    {
        if (existingTimestamp < currentFileTime) // what we have loaded is older
            return false;
    }

    return true;
}

ResourceHandle ResourceLoader::loadResource(const ResourcePath& path)
{
    auto scopeLock = CreateLock(m_lock);

    // TODO: use rename journal to translate path


    // get the loaded resource, that's the most common case so do it first
    // NOTE: if this is disabled we will always create the job to process the resource
    ResourcePtr existingLoadedResource;
    {
        RefPtr<LoadedResource> entry;
        if (m_loadedResources.find(path, entry))
        {
            existingLoadedResource = entry->loadedResource.lock();

            // use existing resource only if timestamp hasn't change since last load
            if (existingLoadedResource && validateResource(path, entry->timestamp))
                return existingLoadedResource;
        }
    }

    // lookup the resource from the loaded list
    // NOTE: there may be different resources loadable from a given file
    {
        RefWeakPtr<LoadingJob> weakJobRef;
        if (m_loadingJobs.find(path, weakJobRef))
        {
            // get the lock to the loading job
            auto loadingJob = weakJobRef.lock();
            if (loadingJob) // there's an active job for this resource key
            {
                scopeLock.release();

                // wait for the job to finish
                Fibers::GetInstance().waitForCounterAndRelease(loadingJob->signal);

                // return the resource pointer produced by the job
                return loadingJob->loadedResource;
            }
        }
    }

    // there's no loading job, create one, this will gate all other threads to wait for us to finish
    auto loadingJob = RefNew<LoadingJob>();
    loadingJob->path = path;
    loadingJob->signal = Fibers::GetInstance().createCounter("LoadingJob", 1);
    m_loadingJobs[path] = loadingJob;

    // unlock the system so other threads can start other loading jobs or join waiting on the one we just created
    scopeLock.release();

    // notify anybody interested
    notifyResourceLoading(path);

    // ask the raw loader to load the content of the resource
    io::TimeStamp loadTimestamp;
    if (auto resource = loadResourceOnce(path, loadTimestamp))
    {
        ASSERT(resource->path() == path);

        // add for safe keeping so we can return it
        // NOTE: we may decide to reuse the same resource, but it does not change the logic here
        {
            auto scopeLock2 = CreateLock(m_lock);

            auto entry = RefNew<LoadedResource>();
            entry->path = path;
            entry->loadedResource = resource;
            entry->timestamp = loadTimestamp;

            m_loadedResources[path] = entry;
        }

        // signal the job as finished, this will unblock other threads
        ASSERT_EX(!loadingJob->loadedResource, "Resource aparently already loaded");
        loadingJob->loadedResource = resource;

        // notify loader
        notifyResourceLoaded(path, resource);

        // post a reload
        if (existingLoadedResource)
            notifyResourceReloaded(existingLoadedResource, resource);
    }
    else
    {
        // whoops
        notifyResourceFailed(path);
    }

    // finish with result
    Fibers::GetInstance().signalCounter(loadingJob->signal);
    return loadingJob->loadedResource;
}

bool ResourceLoader::acquireLoadedResource(const ResourcePath& path, ResourcePtr& outLoadedPtr)
{
    auto lock = CreateLock(m_lock);

    // TODO: use rename journal to translate path

    // get the loaded resource, that's the most common case so do it first
    {
        RefPtr<LoadedResource> entry;
        if (m_loadedResources.find(path, entry))
        {
            // use existing resource only if timestamp hasn't change since last load
            auto existingLoadedResource = entry->loadedResource.lock();
            if (existingLoadedResource && validateResource(path, entry->timestamp))
                return existingLoadedResource;
        }
    }            

    // are we loading it ?
    RefWeakPtr<LoadingJob> weakJobRef;
    if (m_loadingJobs.find(path, weakJobRef))
    {
        // get the lock to the loading job
        if (auto loadingJob = weakJobRef.lock())
        {
            outLoadedPtr = loadingJob->loadedResource;
            return true;
        }
    }

    // file not found as loaded
    return false;
}

//--

ResourceHandle ResourceLoader::loadResourceOnce(const ResourcePath& path, io::TimeStamp& outTimestamp)
{
    if (!GetService<DepotService>()->queryFileTimestamp(path.view(), outTimestamp))
        return nullptr;

    auto file = GetService<DepotService>()->createFileAsyncReader(path.view());
    if (!file)
        return nullptr;

    FileLoadingContext context;
    context.resourceLoadPath = path;
    context.resourceLoader = this;

    if (LoadFile(file, context))
        return context.root<IResource>();

    return nullptr;
}

//--

void ResourceLoader::applyReload(ResourcePtr currentResource, ResourcePtr newResource)
{
    DEBUG_CHECK_RETURN_EX(currentResource, "Invalid current resource");
    DEBUG_CHECK_RETURN_EX(newResource, "Invalid target resource");

    ScopeTimer timer;

    TRACE_INFO("Applying reload to '{}'", currentResource->path());
    //currentResource->applyReload(newResource);

    uint32_t numObjectsVisited = 0;
    Array<ObjectPtr> affectedObjects;
    ObjectGlobalRegistry::GetInstance().iterateAllObjects([&numObjectsVisited, &affectedObjects, &currentResource, &newResource](IObject* obj)
        {
            numObjectsVisited += 1;

            if (obj->onResourceReloading(currentResource, newResource))
                affectedObjects.pushBack(ObjectPtr(AddRef(obj)));

            return false;
        });

    for (const auto obj : affectedObjects)
        obj->onResourceReloadFinished(currentResource, newResource);

    IStaticResource::ApplyReload(currentResource, newResource);

    /*if (const auto depotPath = currentResource->path().str())
        DispatchGlobalEvent(m_depot->eventKey(), EVENT_DEPOT_FILE_RELOADED, depotPath);*/

    TRACE_INFO("Reload to '{}' applied in {}, {} of {} objects pached", currentResource->path(), timer, affectedObjects.size(), numObjectsVisited);
}

bool ResourceLoader::popNextReload(PendingReload& outReload)
{
    auto lock = CreateLock(m_pendingReloadQueueLock);

    if (m_pendingReloadQueue.empty())
        return false;

    outReload = m_pendingReloadQueue.top();
    m_pendingReloadQueue.pop();
    return true;
}

void ResourceLoader::processReloadEvents()
{
    DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Reloading can only happen on main thread");

    PendingReload reload;
    while (popNextReload(reload))
        applyReload(reload.currentResource, reload.newResource);
}

void ResourceLoader::notifyResourceReloaded(const ResourceHandle& currentResource, const ResourceHandle& newResource)
{
    DEBUG_CHECK_RETURN_EX(currentResource, "Invalid current resource");
    DEBUG_CHECK_RETURN_EX(newResource, "Invalid target resource");

    auto lock = CreateLock(m_pendingReloadQueueLock);

    PendingReload info;
    info.currentResource = currentResource;
    info.newResource = newResource;
    m_pendingReloadQueue.push(info);
}

//--

void ResourceLoader::notifyResourceLoading(const ResourcePath& path)
{
    DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_LOADING, path);
}

void ResourceLoader::notifyResourceFailed(const ResourcePath& path)
{
    DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_FAILED, path);
}

void ResourceLoader::notifyResourceLoaded(const ResourcePath& path, const ResourceHandle& data)
{
    DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_LOADED, data);
}

void ResourceLoader::notifyResourceUnloaded(const ResourcePath& path)
{
    DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_UNLOADED, path);
}

//---

END_BOOMER_NAMESPACE_EX(res)
