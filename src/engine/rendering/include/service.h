/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "core/app/include/localService.h"
#include "core/app/include/commandline.h"
#include "core/system/include/rwLock.h"

#include "gpu/device/include/shaderReloadNotifier.h"

BEGIN_BOOMER_NAMESPACE()

///---

struct FrameStats;
class FrameResources;
class FrameHelper;

///---

/// service that facilitates rendering a single scene frame
class ENGINE_RENDERING_API FrameRenderingService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(FrameRenderingService, IService);

public:
    FrameRenderingService();
    ~FrameRenderingService();

    //--

    /// render given scene
    gpu::CommandBuffer* render(const FrameParams& frame, const gpu::AcquiredOutput& output, RenderingScene* scene, const DebugGeometryCollector* debug, FrameStats& outStats);

    //--

private:
    virtual bool onInitializeService(const CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

	FrameResources* m_sharedResources = nullptr;
	FrameHelper* m_sharedHelpers = nullptr;

    gpu::ShaderReloadNotifier m_reloadNotifier;

    void recreateHelpers();
};

///---

END_BOOMER_NAMESPACE()
