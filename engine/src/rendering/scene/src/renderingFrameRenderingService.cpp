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
#include "renderingFrameView_Main.h"

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
            m_surfaceCache = MemNew(FrameSurfaceCache);
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
                    renderer.prepareFrame(cmd);

                    FrameView_Main view(renderer, frame.camera.camera, m_surfaceCache->m_sceneFullColorRT, m_surfaceCache->m_sceneFullDepthRT, frame.mode);
                    view.render(cmd);

                    FinalCopy(cmd, view.width(), view.height(), m_surfaceCache->m_sceneFullColorRT, targetWidth, targetHeight, targetView, 1.0f / 2.2f);
                }

                return cmd.release();
            }
        }
        
        //--

    } // scene
} // rendering