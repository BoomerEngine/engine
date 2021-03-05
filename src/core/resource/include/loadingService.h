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
#include "reference.h"

BEGIN_BOOMER_NAMESPACE()

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
    CAN_YIELD ResourcePtr loadResource(StringView path, ClassType expectedClassType);

    /// load resource from specified path and of specified class (can be subclass)
    /// NOTE: this will yield the current job until the resource is loaded
    CAN_YIELD ResourcePtr loadResource(const ResourceID& id, ClassType expectedClassType);

    //--

protected:
    virtual app::ServiceInitializationResult onInitializeService( const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    RefPtr<ResourceLoader> m_resourceLoader;

    //---

    Array<ResourcePtr> m_retainedFiles;
    SpinLock m_retainedFilesLock;

    void releaseRetainedFiles();
    void addRetainedFile(IResource* file);

    //--
};

//--

END_BOOMER_NAMESPACE()
