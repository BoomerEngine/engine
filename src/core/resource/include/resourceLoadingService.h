/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resource #]
***/

#pragma once

#include "core/app/include/localService.h"
#include "core/containers/include/queue.h"
#include "resourceReference.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

//---

// resource loading service
class CORE_RESOURCE_API LoadingService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(LoadingService, app::ILocalService);

public:
    LoadingService();
            
    //--

    /// created resource loader
    INLINE ResourceLoader* loader() const { return m_resourceLoader; }

    //--

    /// load resource from specified path and of specified class (can be subclass)
    /// NOTE: this will yield the current job until the resource is loaded
    CAN_YIELD ResourcePtr loadResource(const ResourcePath& key);

    /// nice helper for async loading of resources if the resource exists it's returned right away without any extra fibers created (it's the major performance win)
    /// if resource does not exist it's queued for loading and internal fiber is created to service it
    /// NOTE: if the resource exists at the moment of the call the callback function is called right away
    void loadResourceAsync(const ResourcePath& path, const std::function<void(const ResourcePtr&)>& funcLoaded);

    //--

    /// check if resource is in use
    bool acquireLoadedResource(const ResourcePath& path, ResourcePtr& outLoadedPtr);

protected:
    virtual app::ServiceInitializationResult onInitializeService( const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    RefPtr<ResourceLoader> m_resourceLoader;

    //---

    struct AsyncLoadingJob : public IReferencable
    {
        ResourcePtr m_loadedResource;
        FiberSemaphore m_signal;
    };

    HashMap<ResourcePath, RefWeakPtr<AsyncLoadingJob>> m_asyncLoadingJobsMap;
    SpinLock m_asyncLoadingJobsMapLock;

    //---

    struct RetainedFile
    {
        NativeTimePoint m_expiration;
        ResourcePtr m_ptr;
    };

    Queue<RetainedFile> m_retainedFiles;
    SpinLock m_retainedFilesLock;

    void addRetainedFile(const ResourcePtr& file);
    void releaseRetainedFiles();

    //--
};

//--

END_BOOMER_NAMESPACE_EX(res)
