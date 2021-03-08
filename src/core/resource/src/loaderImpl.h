/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resource #]
***/

#pragma once

#include "loader.h"

#include "core/containers/include/queue.h"
#include "core/io/include/timestamp.h"

BEGIN_BOOMER_NAMESPACE()

//---

// resource loading service
class LoadingServiceImpl : public LoadingService
{
    RTTI_DECLARE_VIRTUAL_CLASS(LoadingServiceImpl, LoadingService);

public:
    LoadingServiceImpl();
    virtual ~LoadingServiceImpl();
            
    //--

    /// load resource from specified path and of specified class (can be subclass)
    /// NOTE: this will yield the current job until the resource is loaded
    virtual CAN_YIELD ResourcePtr loadResource(StringView path, ClassType expectedClassType) override final;

    /// load resource from specified path and of specified class (can be subclass)
    /// NOTE: this will yield the current job until the resource is loaded
    virtual CAN_YIELD ResourcePtr loadResource(const ResourceID& id, ClassType expectedClassType) override final;

    //--

protected:
    virtual app::ServiceInitializationResult onInitializeService( const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

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
    void notifyResourceLoaded(StringView path);
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

    void processReloadEvents();

    //---

    HashSet<ResourcePtr> m_retainedFiles;
    SpinLock m_retainedFilesLock;

    void releaseRetainedFiles();
    void addRetainedFile(IResource* file);

    //--
};

//--

END_BOOMER_NAMESPACE()
