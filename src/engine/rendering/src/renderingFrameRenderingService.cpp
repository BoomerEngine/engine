/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "renderingFrameParams.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameRenderingService.h"
#include "renderingFrameResources.h"
#include "renderingScene.h"

#include "renderingFrameView_Main.h"
#include "renderingFrameView_Wireframe.h"
#include "renderingFrameView_CaptureSelection.h"
#include "renderingFrameView_CaptureDepth.h"

#include "gpu/device/include/renderingDeviceService.h"
#include "gpu/device/include/renderingCommandBuffer.h"
#include "core/resource/include/resourceLoadingService.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

RTTI_BEGIN_TYPE_CLASS(FrameRenderingService);
    RTTI_METADATA(app::DependsOnServiceMetadata).dependsOn<res::LoadingService>();
    RTTI_METADATA(app::DependsOnServiceMetadata).dependsOn<DeviceService>();
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

app::ServiceInitializationResult FrameRenderingService::onInitializeService(const app::CommandLine& cmdLine)
{
    auto device = GetService<DeviceService>()->device();

    m_sharedResources = new FrameResources(device);
    m_sharedHelpers = new FrameHelper(device);

    m_reloadNotifier = [this]() { recreateHelpers(); };

    return app::ServiceInitializationResult::Finished;
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

gpu::CommandBuffer* FrameRenderingService::renderFrame(const FrameParams& frame, const FrameCompositionTarget& targetView, FrameStats* outFrameStats, SceneStats* outMergedStateStats)
{
    PC_SCOPE_LVL0(RenderFrame);

    // adjust surfaces to support the requires resolution
    const auto requiredWidth = frame.resolution.width;
    const auto requiredHeight = frame.resolution.height;
	m_sharedResources->adjust(requiredWidth, requiredHeight);

	// rendering block
	gpu::CommandWriter cmd("RenderFrame");

	{
		FrameRenderer renderer(frame, targetView, *m_sharedResources, *m_sharedHelpers);

        renderer.prepareFrame(cmd);

        if (frame.capture.mode == FrameCaptureMode::SelectionRect)
        {
            FrameViewCaptureSelection::Setup setup;
            setup.camera = frame.camera.camera;
            setup.viewport = targetView.targetRect;
            setup.captureRegion = frame.capture.region;
            setup.captureSink = frame.capture.sink;

            FrameViewCaptureSelection view(renderer, setup);
            view.render(cmd);
        }
        else if (frame.capture.mode == FrameCaptureMode::DepthRect)
        {
            FrameViewCaptureDepth::Setup setup;
            setup.camera = frame.camera.camera;
            setup.viewport = targetView.targetRect;
            setup.captureRegion = frame.capture.region;
            setup.captureSink = frame.capture.sink;

            FrameViewCaptureDepth view(renderer, setup);
            view.render(cmd);
        }
        else if (frame.mode == FrameRenderMode::WireframePassThrough || frame.mode == FrameRenderMode::WireframeSolid)
        {
            FrameViewWireframe::Setup setup;
            setup.camera = frame.camera.camera;
            setup.colorTarget = targetView.targetColorRTV;
            setup.depthTarget = targetView.targetDepthRTV;
            setup.viewport = targetView.targetRect;

            FrameViewWireframe view(renderer, setup);
            view.render(cmd);
        }
        else if (frame.mode == FrameRenderMode::Default)
        {
            FrameViewMain::Setup setup;
            setup.camera = frame.camera.camera;
            setup.colorTarget = targetView.targetColorRTV;
            setup.depthTarget = targetView.targetDepthRTV;
            setup.viewport = targetView.targetRect;

            FrameViewMain view(renderer, setup);
            view.render(cmd);
        }

        {
            /*// render main camera view
                    

            // post process the result
            FinalCopy(cmd, view.width(), view.height(), m_surfaceCache->m_sceneFullColorRT, targetWidth, targetHeight, targetView, 1.0f / 2.2f);

            // regardless of the rendering mode draw the depth buffer with selected fragments so we can compose a selection outline
            if (frame.filters & FilterBit::PostProcesses_SelectionHighlight || frame.filters & FilterBit::PostProcesses_SelectionOutline)
            {
                gpu::CommandWriter localCmd(cmd.opCreateChildCommandBuffer(), "SelectionOutline");

                BindSingleCamera(localCmd, frame.camera.camera);

                const auto selectionDepthBufferRT = renderer.surfaces().m_sceneSelectionDepthRT;
                RenderDepthSelection(localCmd, view, selectionDepthBufferRT);

                VisualizeSelectionOutline(localCmd, view.width(), view.height(), targetView, m_surfaceCache->m_sceneFullDepthRT, m_surfaceCache->m_sceneSelectionDepthRT, frame.selectionOutline);
            }*/

            // draw overlay fragments
            //RenderOverlay(cmd, view, targetView);
        }

        renderer.finishFrame();

        if (outFrameStats)
            outFrameStats->merge(renderer.frameStats());

        if (outMergedStateStats)
            outMergedStateStats->merge(renderer.scenesStats());
    }

			

	return cmd.release();
}

//--

END_BOOMER_NAMESPACE_EX(rendering)