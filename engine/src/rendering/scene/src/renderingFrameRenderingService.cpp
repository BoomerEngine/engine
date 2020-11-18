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
#include "renderingFrameSurfaceCache.h"

#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "renderingFrameView.h"
#include "renderingFrameView_Main.h"
#include "renderingFrameView_Selection.h"
#include "renderingSceneUtils.h"

namespace rendering
{
	namespace scene
	{
        //--

        RTTI_BEGIN_TYPE_CLASS(FrameRenderingService);
            RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<DeviceService>();
        RTTI_END_TYPE();

        FrameRenderingService::FrameRenderingService()
        {
        }

        FrameRenderingService::~FrameRenderingService()
        {}

        base::app::ServiceInitializationResult FrameRenderingService::onInitializeService(const base::app::CommandLine& cmdLine)
        {
            auto device = base::GetService<DeviceService>()->device();

            m_surfaceCache = new FrameSurfaceCache(device);
            return base::app::ServiceInitializationResult::Finished;
        }

        void FrameRenderingService::onShutdownService()
        {
            delete m_surfaceCache;
            m_surfaceCache = nullptr;
        }

        void FrameRenderingService::onSyncUpdate()
        {

        }

        command::CommandBuffer* FrameRenderingService::renderFrame(const FrameParams& frame, const ImageView& targetView, FrameStats* outFrameStats, SceneStats* outMergedStateStats)
        {
            PC_SCOPE_LVL0(RenderFrame);

            // no rendering possible without surfaces
            if (!m_surfaceCache)
                return nullptr;

            // adjust surfaces to support the requires resolution
            const auto requiredWidth = frame.resolution.width;
            const auto requiredHeight = frame.resolution.height;
            const auto targetWidth = frame.resolution.finalCompositionWidth;
            const auto targetHeight = frame.resolution.finalCompositionHeight;
            if (!m_surfaceCache->adjust(requiredWidth, requiredHeight))
                return nullptr;

            {
                command::CommandWriter cmd("RenderFrame");

                {
                    FrameRenderer renderer(frame, *m_surfaceCache);
                    {
                        renderer.prepareFrame(cmd);

                        if (frame.capture.mode == FrameCaptureMode::Disabled)
                        {
                            // render main camera view
                            FrameView_Main view(renderer, frame.camera.camera, m_surfaceCache->m_sceneFullColorRT, m_surfaceCache->m_sceneFullDepthRT, frame.mode);
                            view.render(cmd);

                            // post process the result
                            FinalCopy(cmd, view.width(), view.height(), m_surfaceCache->m_sceneFullColorRT, targetWidth, targetHeight, targetView, 1.0f / 2.2f);

                            // regardless of the rendering mode draw the depth buffer with selected fragments so we can compose a selection outline
                            if (frame.filters & FilterBit::PostProcesses_SelectionHighlight || frame.filters & FilterBit::PostProcesses_SelectionOutline)
                            {
                                command::CommandWriter localCmd(cmd.opCreateChildCommandBuffer(), "SelectionOutline");

                                BindSingleCamera(localCmd, frame.camera.camera);

                                const auto selectionDepthBufferRT = renderer.surfaces().m_sceneSelectionDepthRT;
                                RenderDepthSelection(localCmd, view, selectionDepthBufferRT);

                                VisualizeSelectionOutline(localCmd, view.width(), view.height(), targetView, m_surfaceCache->m_sceneFullDepthRT, m_surfaceCache->m_sceneSelectionDepthRT, frame.selectionOutline);
                            }

                            // draw overlay fragments
                            RenderOverlay(cmd, view, targetView);

                        }
                        else if (frame.capture.mode == FrameCaptureMode::SelectionRect && frame.capture.dataBuffer)
                        {
                            FrameView_Selection view(renderer, frame.camera.camera, m_surfaceCache->m_sceneFullDepthRT, frame.capture.area, frame.capture.dataBuffer);
                            view.render(cmd);
                        }

                        renderer.finishFrame();
                    }

                    if (outFrameStats)
                        outFrameStats->merge(renderer.frameStats());

                    if (outMergedStateStats)
                        outMergedStateStats->merge(renderer.scenesStats());
                }

                return cmd.release();
            }
        }
        
        //--

    } // scene
} // rendering