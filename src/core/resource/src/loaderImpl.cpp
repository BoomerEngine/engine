/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#include "build.h"
#include "resource.h"
#include "loader.h"
#include "loaderImpl.h"
#include "depot.h"
#include "fileLoader.h"

#include "core/io/include/timestamp.h"
#include "core/object/include/objectGlobalRegistry.h"
#include "core/containers/include/path.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(LoadingServiceImpl);
RTTI_END_TYPE();

//--

LoadingServiceImpl::LoadingServiceImpl()
{
    m_eventKey = MakeUniqueEventKey("LoadingServiceImpl");
}

LoadingServiceImpl::~LoadingServiceImpl()
{
}

bool LoadingServiceImpl::onInitializeService(const CommandLine& cmdLine)
{
    // TODO: resource preload ?

    return true;
}

void LoadingServiceImpl::onShutdownService()
{
}

void LoadingServiceImpl::onSyncUpdate()
{
    processReloadEvents();
    releaseRetainedFiles();
}

//--

bool LoadingServiceImpl::validateResource(StringView key, const TimeStamp& existingTimestamp)
{
    TimeStamp currentFileTime;
    if (GetService<DepotService>()->queryFileTimestamp(key, currentFileTime))
    {
        if (existingTimestamp < currentFileTime) // what we have loaded is older
            return false;
    }

    return true;
}

static char ConformChar(char ch)
{
    if (ch <= ' ') return ' ';
    if (ch == '\\') return '/';
    if (ch >= 'A' && ch <= 'Z')
        return (ch - 'A') + 'a';
    return ch;
}

LoadingServiceImpl::FileKey::FileKey(StringView text)
{
    CRC64 hashLo, hashHi;

    for (const auto ch : text)
        hashLo << ConformChar(ch);

    {
        const auto* start = text.data() + text.length();
        const auto* end = text.data();
        while (start > end)
        {
            auto ch = ConformChar(*(--start));
            hashHi << ch;
        }
    }

    lo = hashLo;
    hi = hashHi;
}

ResourcePtr LoadingServiceImpl::loadResource(StringView path, ClassType expectedClass)
{
    // compute file key
    const auto fileKey = FileKey(path);

    auto scopeLock = CreateLock(m_lock);

    // get the loaded resource, that's the most common case so do it first
    // NOTE: if this is disabled we will always create the job to process the resource
    ResourcePtr existingLoadedResource;
    {
        RefPtr<LoadedResource> entry;
        if (m_loadedResources.find(fileKey, entry))
        {
            existingLoadedResource = entry->loadedResource.lock();

            // use existing resource only if timestamp hasn't change since last load
            if (existingLoadedResource && validateResource(entry->loadPath, entry->timestamp))
            {
                addRetainedFile(existingLoadedResource);
                return existingLoadedResource;
            }
        }
    }

    // lookup the resource from the loaded list
    // NOTE: there may be different resources loadable from a given file
    {
        RefWeakPtr<LoadingJob> weakJobRef;
        if (m_loadingJobs.find(fileKey, weakJobRef))
        {
            // get the lock to the loading job
            auto loadingJob = weakJobRef.lock();
            if (loadingJob) // there's an active job for this resource key
            {
                scopeLock.release();

                // wait for the job to finish
                WaitForFence(loadingJob->signal);

                // return the resource pointer produced by the job
                addRetainedFile(loadingJob->loadedResource);
                return loadingJob->loadedResource;
            }
        }
    }

    // there's no loading job, create one, this will gate all other threads to wait for us to finish
    auto loadingJob = RefNew<LoadingJob>();
    loadingJob->key = fileKey;
    loadingJob->loadPath = StringBuf(path);
    loadingJob->signal = CreateFence("LoadingJob", 1);
    m_loadingJobs[path] = loadingJob;

    // unlock the system so other threads can start other loading jobs or join waiting on the one we just created
    scopeLock.release();

    // notify anybody interested
    notifyResourceLoading(path);

    // ask the raw loader to load the content of the resource
    TimeStamp loadTimestamp;
    ResourcePtr loadedResource;
    if (GetService<DepotService>()->loadFileToResource(path, loadedResource, true, &loadTimestamp))
    {
        // add for safe keeping so we can return it
        // NOTE: we may decide to reuse the same resource, but it does not change the logic here
        {
            auto scopeLock2 = CreateLock(m_lock);

            auto entry = RefNew<LoadedResource>();
            entry->key = fileKey;
            entry->loadPath = StringBuf(path);
            entry->loadedResource = loadedResource;
            entry->timestamp = loadTimestamp;

            m_loadedResources[path] = entry;
        }

        // signal the job as finished, this will unblock other threads
        ASSERT_EX(!loadingJob->loadedResource, "Resource aparently already loaded");
        loadingJob->loadedResource = loadedResource;

        // notify loader
        notifyResourceLoaded(path);

        // post a reload
        if (existingLoadedResource)
            notifyResourceReloaded(existingLoadedResource, loadedResource);
    }
    else
    {
        // whoops
        notifyResourceFailed(path);
    }

    // finish with result
    SignalFence(loadingJob->signal);
    addRetainedFile(loadingJob->loadedResource);
    return loadingJob->loadedResource;
}

ResourcePtr LoadingServiceImpl::loadResource(const ResourceID& id, ClassType expectedClassType)
{
    if (!id)
        return nullptr;

    StringBuf loadPath;
    if (!GetService<DepotService>()->resolvePathForID(id, loadPath))
    {
        TRACE_WARNING("No load path for resource '{}'", id);
        return nullptr;
    }

    auto depotPath = ReplaceExtension(loadPath, IResource::FILE_EXTENSION); // tempshit

    return loadResource(depotPath, expectedClassType);
}

#if 0
bool LoadingServiceImpl::acquireLoadedResource(const ResourcePath& path, ResourcePtr& outLoadedPtr)
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
            if (existingLoadedResource && validateResource(path.view(), entry->timestamp))
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
#endif

//--

void LoadingServiceImpl::applyReload(ResourcePtr currentResource, ResourcePtr newResource)
{
    DEBUG_CHECK_RETURN_EX(currentResource, "Invalid current resource");
    DEBUG_CHECK_RETURN_EX(newResource, "Invalid target resource");

    ScopeTimer timer;

    TRACE_INFO("Applying reload to '{}'", currentResource->loadPath());
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

    /*if (const auto depotPath = currentResource->path().str())
        DispatchGlobalEvent(m_depot->eventKey(), EVENT_DEPOT_FILE_RELOADED, depotPath);*/

    TRACE_INFO("Reload to '{}' applied in {}, {} of {} objects pached", currentResource->loadPath(), timer, affectedObjects.size(), numObjectsVisited);
}

bool LoadingServiceImpl::popNextReload(PendingReload& outReload)
{
    auto lock = CreateLock(m_pendingReloadQueueLock);

    if (m_pendingReloadQueue.empty())
        return false;

    outReload = m_pendingReloadQueue.top();
    m_pendingReloadQueue.pop();
    return true;
}

void LoadingServiceImpl::processReloadEvents()
{
    DEBUG_CHECK_RETURN_EX(IsMainThread(), "Reloading can only happen on main thread");

    PendingReload reload;
    while (popNextReload(reload))
        applyReload(reload.currentResource, reload.newResource);
}

void LoadingServiceImpl::notifyResourceReloaded(const ResourcePtr& currentResource, const ResourcePtr& newResource)
{
    DEBUG_CHECK_RETURN_EX(currentResource, "Invalid current resource");
    DEBUG_CHECK_RETURN_EX(newResource, "Invalid target resource");

    auto lock = CreateLock(m_pendingReloadQueueLock);

    PendingReload info;
    info.currentResource = currentResource;
    info.newResource = newResource;
    m_pendingReloadQueue.push(info);
}

void LoadingServiceImpl::releaseRetainedFiles()
{
    m_retainedFilesLock.acquire();
    m_retainedFiles.reset();
    m_retainedFilesLock.release();
}

void LoadingServiceImpl::addRetainedFile(IResource* file)
{
    if (file)
    {
        auto lock = CreateLock(m_retainedFilesLock);
        m_retainedFiles.insert(AddRef(file));
    }
}

//--

void LoadingServiceImpl::notifyResourceLoading(StringView path)
{
    DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_LOADING, StringBuf(path));
}

void LoadingServiceImpl::notifyResourceFailed(StringView path)
{
    DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_FAILED, StringBuf(path));
}

void LoadingServiceImpl::notifyResourceLoaded(StringView path)
{
    DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_LOADED, StringBuf(path));
}

void LoadingServiceImpl::notifyResourceUnloaded(StringView path)
{
    DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_UNLOADED, StringBuf(path));
}

//---

END_BOOMER_NAMESPACE()
