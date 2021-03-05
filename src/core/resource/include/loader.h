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
    ResourcePtr loadResource(StringView path, ClassType expectedClass = nullptr) CAN_YIELD;

    /// Try to get already loaded resource if it's already loaded
    /// NOTE: this should be used to optimize calls to async loading functions by first querying if resource exists
    /// NOTE: this ONLY returns fully loaded resources (so if the resource is actually being loaded now it's not returned, as the name of the function states)
    bool acquireLoadedResource(StringView path, ResourcePtr& outLoadedPtr);

    //----

    /// Load a resource from specified path, may return NULL resource
    /// NOTE: this function can wait for other jobs and thus yield the active fiber
    template<typename T>
    INLINE CAN_YIELD RefPtr<T> loadResource(StringView path)
    {
        return rtti_cast<T>(loadResource(path, T::GetStatiClass()));
    }

    //----

    /// process pending reload events
    void processReloadEvents();

    ///---

protected:
    GlobalEventKey m_eventKey;

    //--

    struct FileKey
    {
        uint64_t lo = 0;
        uint64_t hi = 0;

        INLINE FileKey() {};
        INLINE FileKey(const FileKey& other) = default;
        INLINE FileKey& operator=(const FileKey& other) = default;

        INLINE static uint32_t CalcHash(const FileKey& key)
        {
            return CRC32() << key.lo << key.hi;
        }

        INLINE bool operator==(const FileKey& key) const
        {
            return (lo == key.lo) && (hi == key.hi);
        }

        FileKey(StringView text);
    };

    //--

    void notifyResourceLoading(StringView path);
    void notifyResourceLoaded(StringView path, const ResourcePtr& data);
    void notifyResourceFailed(StringView path);
    void notifyResourceUnloaded(StringView path);

    //--

    struct LoadedResource : public IReferencable
    {
        FileKey key;
        StringBuf loadPath;
        TimeStamp timestamp;

        RefWeakPtr<IResource> loadedResource;
    };

    struct LoadingJob : public IReferencable
    {
        FiberSemaphore signal;

        FileKey key;
        StringBuf loadPath;
        ResourcePtr loadedResource;
    };

    SpinLock m_lock;

    typedef HashMap<FileKey, RefWeakPtr<LoadingJob>> TLoadingJobMap;
    TLoadingJobMap m_loadingJobs; // map for active loading jobs

    typedef HashMap<FileKey, RefPtr<LoadedResource>> TResourceMap;
    TResourceMap m_loadedResources; // map for active loading jobs

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
