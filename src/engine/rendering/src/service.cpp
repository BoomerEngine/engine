/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "params.h"
#include "renderer.h"
#include "service.h"
#include "resources.h"
#include "scene.h"

#include "viewMain.h"
#include "viewWireframe.h"
#include "viewCaptureSelection.h"
#include "viewCaptureDepth.h"

#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/commandBuffer.h"
#include "core/resource/include/loader.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

RTTI_BEGIN_TYPE_CLASS(FrameRenderingService);
    //RTTI_METADATA(DependsOnServiceMetadata).dependsOn<LoadingService>();
    RTTI_METADATA(DependsOnServiceMetadata).dependsOn<DeviceService>();
RTTI_END_TYPE();

FrameRenderingService::FrameRenderingService()
{
}

FrameRenderingService::~FrameRenderingService()
{}

void FrameRenderingService::recreateHelpers()
{
    auto device = GetService<DeviceService>()->device();

    delete m_sharedHelpers;
    m_sharedHelpers = new FrameHelper(device);
}

bool FrameRenderingService::onInitializeService(const CommandLine& cmdLine)
{
    auto device = GetService<DeviceService>()->device();

    m_sharedResources = new FrameResources(device);
    m_sharedHelpers = new FrameHelper(device);

    m_reloadNotifier = [this]() { recreateHelpers(); };

    return true;
}

void FrameRenderingService::onShutdownService()
{
    delete m_sharedHelpers;
    m_sharedHelpers = nullptr;

    delete m_sharedResources;
	m_sharedResources = nullptr;
}

void FrameRenderingService::onSyncUpdate()
{

}

gpu::CommandBuffer* FrameRenderingService::render(const FrameParams& frame, const gpu::AcquiredOutput& output, Scene* scene, FrameStats& outStats)
{
    PC_SCOPE_LVL0(RenderFrame);

    ScopeTimer timer;

    // adjust surfaces to support the requires resolution
    const auto requiredWidth = frame.resolution.width;
    const auto requiredHeight = frame.resolution.height;
	m_sharedResources->adjust(requiredWidth, requiredHeight);

    // lock scene for rendering
    if (scene)
        scene->renderLock();

	// rendering block
    {
        gpu::CommandWriter cmd("RenderFrame");

        // create frame renderer
        FrameRenderer renderer(frame, output, *m_sharedResources, *m_sharedHelpers, scene);

        // prepare renderer
        renderer.prepare(cmd);

        // prepare scene for rendering
        if (scene)
            scene->prepare(cmd, renderer);

        //  render scene
        if (frame.capture.mode == FrameCaptureMode::SelectionRect)
        {
            FrameViewCaptureSelection::Setup setup;
            setup.camera = frame.camera.camera;
            setup.viewport = Rect(0, 0, output.width, output.height);
            setup.captureRegion = frame.capture.region;
            setup.captureSink = frame.capture.sink;

            FrameViewCaptureSelection view(renderer, setup);
            view.render(cmd);
        }
        else if (frame.capture.mode == FrameCaptureMode::DepthRect)
        {
            FrameViewCaptureDepth::Setup setup;
            setup.camera = frame.camera.camera;
            setup.viewport = Rect(0, 0, output.width, output.height);
            setup.captureRegion = frame.capture.region;
            setup.captureSink = frame.capture.sink;

            FrameViewCaptureDepth view(renderer, setup);
            view.render(cmd);
        }
        else if (frame.mode == FrameRenderMode::WireframePassThrough || frame.mode == FrameRenderMode::WireframeSolid)
        {
            FrameViewWireframe::Setup setup;
            setup.camera = frame.camera.camera;
            setup.colorTarget = output.color;
            setup.depthTarget = output.depth;
            setup.viewport = Rect(0, 0, output.width, output.height);

            FrameViewWireframe view(renderer, setup);
            view.render(cmd);
        }
        else if (frame.mode == FrameRenderMode::Default)
        {
            FrameViewMain::Setup setup;
            setup.camera = frame.camera.camera;
            setup.colorTarget = output.color;
            setup.depthTarget = output.depth;
            setup.viewport = Rect(0, 0, output.width, output.height);

            FrameViewMain view(renderer, setup);
            view.render(cmd);
        }

        // finish scene frame
        if (scene)
            scene->finish(cmd, renderer, outStats);

        // finish renderer frame
        renderer.finish(cmd, outStats);

        // export command buffer
        outStats.totals.merge(outStats.depthView);
        outStats.totals.merge(outStats.mainView);
        outStats.totals.merge(outStats.globalShadowView);
        outStats.totals.merge(outStats.localShadowView);
        outStats.totalTime = timer.timeElapsed();

        // release scene render lock
        if (scene)
            scene->renderUnlock();

        return cmd.release();
    }    
}

//--

END_BOOMER_NAMESPACE_EX(rendering)