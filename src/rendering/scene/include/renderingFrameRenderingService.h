/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "base/app/include/localService.h"
#include "base/app/include/commandline.h"
#include "base/system/include/rwLock.h"

#include "rendering/device/include/renderingShaderReloadNotifier.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

///---

struct SceneStats;
struct FrameStats;
class FrameResources;
class FrameHelper;

///---

/// composition target for the frame
struct FrameCompositionTarget
{
	const RenderTargetView* targetColorRTV = nullptr;
	const RenderTargetView* targetDepthRTV = nullptr;
	base::Rect targetRect;
};

///---

/// service that facilitates rendering a single scene frame
class RENDERING_SCENE_API FrameRenderingService : public base::app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(FrameRenderingService, base::app::ILocalService);

public:
    FrameRenderingService();
    ~FrameRenderingService();

    //--

    /// render command buffers for rendering given frame
    GPUCommandBuffer* renderFrame(const FrameParams& frame, const FrameCompositionTarget& target, FrameStats* outFrameStats = nullptr, SceneStats* outMergedStateStats = nullptr);

    //--

private:
    virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

	FrameResources* m_sharedResources = nullptr;
	FrameHelper* m_sharedHelpers = nullptr;

    ShaderReloadNotifier m_reloadNotifier;

    void recreateHelpers();
};

///---

END_BOOMER_NAMESPACE(rendering::scene)