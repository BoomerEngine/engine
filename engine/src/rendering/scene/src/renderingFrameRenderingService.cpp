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

#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingCommandBuffer.h"
#include "renderingFrameView.h"
#include "renderingFrameViewCamera.h"
#include "renderingFrameViewPostFX.h"

namespace rendering
{
	namespace scene
	{
        //--

        RTTI_BEGIN_TYPE_CLASS(FrameRenderingService);
        RTTI_END_TYPE();

        FrameRenderingService::FrameRenderingService()
        {}

        FrameRenderingService::~FrameRenderingService()
        {}

        base::app::ServiceInitializationResult FrameRenderingService::onInitializeService(const base::app::CommandLine& cmdLine)
        {
            return base::app::ServiceInitializationResult::Finished;
        }

        void FrameRenderingService::onShutdownService()
        {
            MemDelete(m_surfaceCache);
            m_surfaceCache = nullptr;
        }

        void FrameRenderingService::onSyncUpdate()
        {

        }

        command::CommandBuffer* FrameRenderingService::renderFrame(const FrameParams& frame, const ImageView& targetView)
        {
            PC_SCOPE_LVL0(RenderFrame);

            auto* device = base::GetService<DeviceService>()->device();

            if (!m_surfaceCache || !m_surfaceCache->supports(frame.resolution))
            {
                MemDelete(m_surfaceCache);
                m_surfaceCache = MemNew(FrameSurfaceCache, frame.resolution);
            }

            {
                command::CommandWriter cmd("RenderFrame");

                {
                    FrameRenderer renderer(device, frame, *m_surfaceCache);
                    renderer.prepareFrame(cmd);

                    FrameViewCamera mainCamera(frame);
                    mainCamera.calcMainCamera();

                    if (frame.mode == scene::FrameRenderMode::Default)
                    {
                        const auto& resolvedColor = renderer.fetchImage(FrameResource::HDRResolvedColor);
                        const auto& resolvedDepth = renderer.fetchImage(FrameResource::HDRResolvedDepth);

                        RenderLitView(cmd, renderer, mainCamera, resolvedColor, resolvedDepth);

                        /*if (frame.filters & FilterBit::PostProcessing)
                        {

                        }*/

                        PostFxTargetBlit(cmd, renderer, resolvedColor, targetView);
                    }
                    else if (frame.mode == scene::FrameRenderMode::WireframeSolid)
                    {
                        const auto& resolvedColor = renderer.fetchImage(FrameResource::HDRResolvedColor);

                        RenderWireframeView(cmd, renderer, mainCamera, resolvedColor, true);

                        PostFxTargetBlit(cmd, renderer, resolvedColor, targetView, false);
                    }
                    else if (frame.mode == scene::FrameRenderMode::WireframePassThrough)
                    {
                        const auto& resolvedColor = renderer.fetchImage(FrameResource::HDRResolvedColor);

                        RenderWireframeView(cmd, renderer, mainCamera, resolvedColor, false);

                        PostFxTargetBlit(cmd, renderer, resolvedColor, targetView, false);
                    }
                    else if (frame.mode == scene::FrameRenderMode::DebugDepth)
                    {
                        RenderDepthDebug(cmd, renderer, mainCamera, targetView);
                    }
                    else if (frame.mode == scene::FrameRenderMode::DebugLuminance)
                    {
                        RenderLuminanceDebug(cmd, renderer, mainCamera, targetView);
                    }
                }

                return cmd.release();
            }
        }
        
        //--

    } // scene
} // rendering