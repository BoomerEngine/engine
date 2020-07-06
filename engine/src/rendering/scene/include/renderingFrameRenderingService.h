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
#include "base/io/include/absolutePath.h"
#include "base/system/include/rwLock.h"

namespace rendering
{
    namespace scene
    {
        ///---

        struct SceneStats;
        struct FrameStats;

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
            command::CommandBuffer* renderFrame(const FrameParams& frame, const ImageView& targetView, FrameStats* outFrameStats = nullptr, SceneStats* outMergedStateStats = nullptr);

            //--

        private:
            virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
            virtual void onShutdownService() override final;
            virtual void onSyncUpdate() override final;

            FrameSurfaceCache* m_surfaceCache = nullptr;
        };

        ///---

    } // scene
} // rendering