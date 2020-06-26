/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/app/include/localService.h"

namespace rendering
{
    namespace canvas
    {

        ///---

        /// helper service for rendering canvas into render targets
        class RENDERING_CANVAS_API CanvasRenderingService : public base::app::ILocalService
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CanvasRenderingService, base::app::ILocalService);

        public:
            CanvasRenderingService();
            virtual ~CanvasRenderingService();

            /// render canvas content into current render pass 
            void render(command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const CanvasRenderingParams& params);

        private:            
            virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
            virtual void onShutdownService() override final;
            virtual void onSyncUpdate() override final;

            CanvasRenderer* m_renderer = nullptr;
        };

        ///---

    } // canvas
} // rendering

typedef rendering::canvas::CanvasRenderingService CanvasService;