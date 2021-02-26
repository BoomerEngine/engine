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

#include "gpu/device/include/renderingShaderReloadNotifier.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///---

struct SceneStats;
struct FrameStats;
class FrameResources;
class FrameHelper;

///---

/// composition target for the frame
struct FrameCompositionTarget
{
	const gpu::RenderTargetView* targetColorRTV = nullptr;
	const gpu::RenderTargetView* targetDepthRTV = nullptr;
	Rect targetRect;
};

///---

/// service that facilitates rendering a single scene frame
class ENGINE_RENDERING_API FrameRenderingService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(FrameRenderingService, app::ILocalService);

public:
    FrameRenderingService();
    ~FrameRenderingService();

    //--

    /// render command buffers for rendering given frame
    gpu::CommandBuffer* renderFrame(const FrameParams& frame, const FrameCompositionTarget& target, FrameStats* outFrameStats = nullptr, SceneStats* outMergedStateStats = nullptr);

    //--

private:
    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

	FrameResources* m_sharedResources = nullptr;
	FrameHelper* m_sharedHelpers = nullptr;

    gpu::ShaderReloadNotifier m_reloadNotifier;

    void recreateHelpers();
};

///---

END_BOOMER_NAMESPACE_EX(rendering)
