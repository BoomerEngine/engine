/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "renderingFrameParams.h"
#include "renderingSceneCulling.h"

#include "rendering/device/include/renderingParametersView.h"
#include "rendering/scene/include/renderingFrameCamera.h"
#include "rendering/device/include/renderingFramebuffer.h"

#include "base/fibers/include/fiberWaitList.h"
#include "base/memory/include/linearAllocator.h"
#include "base/reflection/include/variantTable.h"
#include "base/memory/include/pageCollection.h"
#include "renderingSceneStats.h"

namespace rendering
{
    namespace scene
    {

        ///---

        struct FragmentRenderContext;
        class FrameView;

        ///---

        /// a stack-based renderer created to facilitate rendering of a single frame
        class RENDERING_SCENE_API FrameRenderer : public base::NoCopy
        {
        public:
            FrameRenderer(const FrameParams& frame, const FrameSurfaceCache& surfaces);
            ~FrameRenderer();

            //--

            INLINE base::mem::LinearAllocator& allocator() { return m_allocator; }

            INLINE const FrameParams& frame() const { return m_frame; }

            INLINE const FrameSurfaceCache& surfaces() const { return m_surfaces; }

            INLINE const FrameStats& frameStats() const { return m_frameStats; } // frame only stats
            INLINE const SceneStats& scenesStats() const { return m_mergedSceneStats; } // merged from all scenes

            INLINE uint32_t targetWidth() const { return m_frame.resolution.finalCompositionWidth; }
            INLINE uint32_t targetHeight() const { return m_frame.resolution.finalCompositionHeight; }

            INLINE uint32_t width() const { return m_frame.resolution.width; }
            INLINE uint32_t height() const { return m_frame.resolution.height; }

            //--

            bool usesMultisamping() const;

            //--

            void prepareFrame(command::CommandWriter& cmd);
            void finishFrame();

            //--

        private:
            bool m_msaa = false;

            base::mem::LinearAllocator m_allocator;

            const FrameParams& m_frame;
            const FrameSurfaceCache& m_surfaces;
            
            struct SceneData
            {
                ParametersView params;
                SceneStats stats;
                Scene* scene = nullptr; // TODO: should be const
            };

            base::InplaceArray<SceneData, 10> m_scenes;

            FrameStats m_frameStats;
            SceneStats m_mergedSceneStats;
            
            //--

            friend class FrameView;
        };

        ///---

    } // scene
} // rendering

