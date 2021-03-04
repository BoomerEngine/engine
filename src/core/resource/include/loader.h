/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once

#include "core/containers/include/array.h"
#include "core/containers/include/queue.h"
#include "core/object/include/globalEventKey.h"
#include "core/io/include/timestamp.h"

BEGIN_BOOMER_NAMESPACE()

//-----

/// Resource loader - loads resources files from depot data
class CORE_RESOURCE_API ResourceLoader : public IReferencable
{
public:
    ResourceLoader();
    virtual ~ResourceLoader();

    //----

    /// get the event key, we can use the key to observer for events
    INLINE const GlobalEventKey& eventKey() const { return m_eventKey; }

    //----
    /// Load a resource from specified resource key
    /// NOTE: this function will wait for other jobs and thus yield the active fiber
    /// NOTE: if resource is not found or is invalid a resource stub MAY be returned if allowed, later that stub may get reloaded with valid resource
    ResourcePtr loadResource(const ResourcePath& path) CAN_YIELD;

    /// Try to get already loaded resource if it's already loaded
    /// NOTE: this should be used to optimize calls to async loading functions by first querying if resource exists
    /// NOTE: this ONLY returns fully loaded resources (so if the resource is actually being loaded now it's not returned, as the name of the function states)
    bool acquireLoadedResource(const ResourcePath& path, ResourcePtr& outLoadedPtr);

    //----

    /// Load a resource from specified path, may return NULL resource
    /// NOTE: this function can wait for other jobs and thus yield the active fiber
    template<typename T>
    INLINE CAN_YIELD RefPtr<T> loadResource(StringView path)
    {
        return rtti_cast<T>(loadResource(path));
    }

    //----

    /// process pending reload events
    void processReloadEvents();

    ///---

protected:
    GlobalEventKey m_eventKey;

    //--

    void notifyResourceLoading(const ResourcePath& path);
    void notifyResourceLoaded(const ResourcePath& path, const ResourcePtr& data);
    void notifyResourceFailed(const ResourcePath& path);
    void notifyResourceUnloaded(const ResourcePath& path);

    //--

    struct LoadedResource : public IReferencable
    {
        ResourcePath path;
        TimeStamp timestamp;

        RefWeakPtr<IResource> loadedResource;
    };

    struct LoadingJob : public IReferencable
    {
        FiberSemaphore signal;

        ResourcePath path;
        ResourcePtr loadedResource;
    };

    SpinLock m_lock;

    typedef HashMap<ResourcePath, RefWeakPtr<LoadingJob>> TLoadingJobMap;
    TLoadingJobMap m_loadingJobs;       // map for active loading jobs

    typedef HashMap<ResourcePath, RefPtr<LoadedResource>> TResourceMap;
    TResourceMap m_loadedResources;       // map for active loading jobs

    //--

    ResourcePtr loadResourceOnce(StringView path, TimeStamp& outTimestamp) CAN_YIELD;
    bool validateResource(StringView path, const TimeStamp& existingTimestamp);

    //--

    struct PendingReload
    {
        ResourcePtr currentResource;
        ResourcePtr newResource;
    };

    SpinLock m_pendingReloadQueueLock;
    Queue<PendingReload> m_pendingReloadQueue;

    void notifyResourceReloaded(const ResourcePtr& currentResource, const ResourcePtr& newResource);

    bool popNextReload(PendingReload& outReload);
    void applyReload(ResourcePtr currentResource, ResourcePtr newResource);

    //--

    friend IResource; // for notifyResourceUnloaded
};

//-----

END_BOOMER_NAMESPACE()
