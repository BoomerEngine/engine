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

#include "rendering/driver/include/renderingParametersView.h"
#include "rendering/scene/include/renderingFrameCamera.h"
#include "rendering/driver/include/renderingFramebuffer.h"

#include "base/fibers/include/fiberWaitList.h"
#include "base/memory/include/linearAllocator.h"
#include "base/reflection/include/variantTable.h"
#include "base/memory/include/pageCollection.h"

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

            INLINE uint32_t targetWidth() const { return m_frame.resolution.finalCompositionWidth; }
            INLINE uint32_t targetHeight() const { return m_frame.resolution.finalCompositionHeight; }

            INLINE uint32_t width() const { return m_frame.resolution.width; }
            INLINE uint32_t height() const { return m_frame.resolution.height; }

            //--

            bool usesMultisamping() const;

            //--

            void prepareFrame(command::CommandWriter& cmd);

            //--

        private:
            bool m_msaa = false;

            base::mem::LinearAllocator m_allocator;

            const FrameParams& m_frame;
            const FrameSurfaceCache& m_surfaces;
            
            struct SceneData
            {
                ParametersView params;
                const Scene* scene = nullptr;
            };

            base::InplaceArray<SceneData, 10> m_scenes;
            
            //--

            friend class FrameView;
        };

        ///---

    } // scene
} // rendering

