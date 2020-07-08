/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasRenderer.h"
#include "renderingCanvasRenderingService.h"
#include "renderingCanvasImageCache.h"

#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resourceLoadingService.h"
#include "rendering/driver/include/renderingDeviceService.h"

namespace rendering
{
    namespace canvas
    {
        //---

        RTTI_BEGIN_TYPE_CLASS(CanvasRenderingService);
            RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<base::res::LoadingService>();
            RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<DeviceService>();
        RTTI_END_TYPE();

        CanvasRenderingService::CanvasRenderingService()
        {}

        CanvasRenderingService::~CanvasRenderingService()
        {}

        base::app::ServiceInitializationResult CanvasRenderingService::onInitializeService(const base::app::CommandLine& cmdLine)
        {
            m_renderer = MemNew(CanvasRenderer);
            return base::app::ServiceInitializationResult::Finished;
        }

        void CanvasRenderingService::onShutdownService()
        {
            MemDelete(m_renderer);
            m_renderer = nullptr;
        }

        void CanvasRenderingService::onSyncUpdate()
        {
            // TODO: purge canvas image cache from time to time ?
        }

        //---

        void CanvasRenderingService::render(command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const CanvasRenderingParams& params)
        {
            m_renderer->render(cmd, canvas, params);
        }

        //---

    } // canvas
} // rendering