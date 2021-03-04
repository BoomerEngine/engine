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

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///---

struct SceneStats;
struct FrameStats;
class FrameResources;
class FrameHelper;

///---

/// composition target for the frame
struct ENGINE_RENDERING_API FrameCompositionTarget
{
	const gpu::RenderTargetView* targetColorRTV = nullptr;
	const gpu::RenderTargetView* targetDepthRTV = nullptr;
	Rect targetRect;

    uint32_t width() const;
    uint32_t height() const;
    float aspectRatio() const;
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

    /// render given scene
    gpu::CommandBuffer* render(const FrameParams& frame, const FrameCompositionTarget& target, Scene* scene, FrameStats& outStats);

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
