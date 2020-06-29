/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame  #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"

#pragma once

namespace rendering
{
    namespace scene
    {
        //---

        class FrameRenderer;
        struct CascadeData;

        //---

        /// type of the view
        enum class FrameViewType : uint8_t
        {
            MainColor, // main color view (or derivatives)
            GlobalCascades, // view for global cascades - only shadow casting fragments should be collected
        };

        /// view rendering helper
        class RENDERING_SCENE_API FrameView : public base::NoCopy
        {
        public:
            FrameView(const FrameRenderer& frame, FrameViewType type, uint32_t width, uint32_t height);
            ~FrameView();

            //--

            struct PerSceneData
            {
                const Scene* scene = nullptr;
                ParametersView params;
                SceneObjectCullingResult collectedObjects;
                FragmentDrawList* drawList = nullptr;
            };

            // get view type
            INLINE FrameViewType type() const { return m_type; }

            // rendering width
            INLINE uint32_t width() const { return m_width; }

            // rendering height
            INLINE uint32_t height() const { return m_height; }

            // get per-scene data, we are mostly interested in draw list and list of collected proxies
            INLINE const base::Array<PerSceneData*>& scenes() const { return m_scenes; }

            // parent frame
            INLINE const FrameParams& frame() const { return m_frame; }

            // renderer
            INLINE const FrameRenderer& renderer() const { return m_renderer; }

        protected:
            void collectSingleCamera(const Camera& camera);
            void generateFragments(command::CommandWriter& cmd);

        private:
            FrameViewType m_type;
            const FrameRenderer& m_renderer;
            const FrameParams& m_frame;
            base::mem::LinearAllocator m_allocator;
            base::InplaceArray<PerSceneData*, 10> m_scenes;
            uint32_t m_width = 0;
            uint32_t m_height = 0;
        };

        //---

        // a "bracket" (collection of states) for active pass (PASS = bound render targets)
        struct PassBracket
        {
        public:
            PassBracket(command::CommandWriter& cmd, const FrameView& view, base::StringView<char> name);
            ~PassBracket();

            // bind a depth buffer and clear it
            void depthClear(const ImageView& rt);

            // bind a color buffer and clear it
            void colorClear(uint8_t index, const ImageView& rt, const base::Vector4& clearValues);

            // bind a color buffer and clear it with default color
            void colorClear(uint8_t index, const ImageView& rt);

            // being pass
            void begin();

        private:
            command::CommandWriter& m_cmd;
            const FrameView& m_view;

            FrameBuffer m_fb;
            FrameBufferViewportState m_viewport;

            bool m_hasStartedPass = false;
        };

        //---


        //---------------------------------
        // fragment drawing utilities

        // render debug fragments
        extern RENDERING_SCENE_API void RenderDebugFragments(command::CommandWriter& cmd, const FrameView& view, const DebugGeometry& geom);

        // render scene fragments
        extern RENDERING_SCENE_API void RenderViewFragments(command::CommandWriter& cmd, const FrameView& view, const FragmentRenderContext& context, const std::initializer_list<FragmentDrawBucket>& buckets);

        //---------------------------------
        // pass drawing utilities

        // render depth pre pass
        extern RENDERING_SCENE_API void RenderDepthPrepass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT);

        // render forward pass
        extern RENDERING_SCENE_API void RenderForwardPass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, const ImageView& colorRT);

        // render wireframe pass
        extern RENDERING_SCENE_API void RenderWireframePass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, const ImageView& colorRT, bool solid);

        // render a shadow depth pass
        extern RENDERING_SCENE_API void RenderShadowDepthPass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, uint32_t index = 0);

        //---------------------------------
        // final composition with outside world

        // final copy - simple upscale source rect to target (does not do any color processing)
        extern RENDERING_SCENE_API void FinalCopy(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& source, uint32_t targetWidth, uint32_t targetHeight, const ImageView& target, float gamma=1.0f);

        // final composition - tone map, sharpen, color grade, upscale
        extern RENDERING_SCENE_API void FinalComposition(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& sourceColor, uint32_t targetWidth, uint32_t targetHeight, const ImageView& target, const FrameParams_ToneMapping& toneMapping, const FrameParams_ColorGrading& colorGrading);

        //---------------------------------
        // in-frame effects/computations

        // compute global shadow mask (R channel)
        extern RENDERING_SCENE_API void GlobalShadowMask(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& sourceDepth, const ImageView& targetSSAOMAsk);

        //---------------------------------
        // debug visualizations

        // visualize depth buffer
        extern RENDERING_SCENE_API void VisualizeDepthBuffer(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& depthSource, const ImageView& targetColor);

        // visualize raw luminance (no exposure)
        extern RENDERING_SCENE_API void VisualizeLuminance(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& colorSource, const ImageView& targetColor);

        // visualize a channel in the texture
        extern RENDERING_SCENE_API void VisualizeTexture(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& colorSource, const ImageView& targetColor, const base::Vector4& dot);

        //---------------------------------
        // light & shadows

        // bind the lighting data (cascades + lighting grid)
        extern RENDERING_SCENE_API void BindLightingData(command::CommandWriter& cmd, const CascadeData& cascades);

    } // scene
} // rendering

