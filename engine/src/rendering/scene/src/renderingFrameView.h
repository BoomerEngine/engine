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
        struct LightingData;

        //---

        /// view rendering helper
        class RENDERING_SCENE_API FrameView : public base::NoCopy
        {
        public:
            FrameView(const FrameRenderer& frame, FrameViewType type, uint32_t width, uint32_t height);
            virtual ~FrameView();

            //--

            struct PerSceneData
            {
                const Scene* scene = nullptr;
                SceneViewStats* frameStats = nullptr;
                SceneViewStats stats;
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

            // stats
            INLINE FrameViewStats& stats() const { return m_stats; }

        protected:
            void collectSingleCamera(const Camera& camera);
            void collectCascadeCameras(const Camera& viewCamera, const CascadeData& cascades);
            void generateFragments(command::CommandWriter& cmd);

            mutable FrameViewStats m_stats;

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

            // bind a depth buffer without clearing it
            void depthNoClear(const ImageView& rt);

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
        extern RENDERING_SCENE_API void RenderDepthPrepass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, const ImageView& velocityRT);

        // render forward pass
        extern RENDERING_SCENE_API void RenderForwardPass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, const ImageView& colorRT);

        // render wireframe pass
        extern RENDERING_SCENE_API void RenderWireframePass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, const ImageView& colorRT, bool solid);

        // render a shadow depth pass
        extern RENDERING_SCENE_API void RenderShadowDepthPass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, float depthBiasConstant, float depthBiasSlope, float depthBiasClamp, uint32_t index = 0);

        // render to depth buffer fragments that are selected so an outline can be created around them
        extern RENDERING_SCENE_API void RenderDepthSelection(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT);

        // render the selection fragments
        extern RENDERING_SCENE_API void RenderSelectionFragments(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT);

        //---------------------------------
        // final composition with outside world

        // final copy - simple upscale source rect to target (does not do any color processing)
        extern RENDERING_SCENE_API void FinalCopy(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& source, uint32_t targetWidth, uint32_t targetHeight, const ImageView& target, float gamma=1.0f);

        // final composition - tone map, sharpen, color grade, upscale
        extern RENDERING_SCENE_API void FinalComposition(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& sourceColor, uint32_t targetWidth, uint32_t targetHeight, const ImageView& target, const FrameParams_ToneMapping& toneMapping, const FrameParams_ColorGrading& colorGrading);

        //---------------------------------
        // in-frame effects/computations

        // compute global shadow mask (R channel)
        extern RENDERING_SCENE_API void GlobalShadowMask(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& sourceDepth, const ImageView& targetSSAOMask);

        // linearize scene depth, uses bound camera params
        extern RENDERING_SCENE_API void LinearizeDepth(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const Camera& viewCamera, const ImageView& sourceDepth, const ImageView& targetLinearDepth);

        // reconstruct view space normals
        extern RENDERING_SCENE_API void ReconstructViewNormals(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const Camera& viewCamera, const ImageView& sourceLinearizedDepth, const ImageView& reconstructedViewNormal);

        // compute ambient occlusion (HBAO) into G channel
        extern RENDERING_SCENE_API void HBAOIntoShadowMask(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& sourceLinearDepth, const ImageView& targetSSAOMask, const Camera& viewCamera, const FrameParams_AmbientOcclusion& setup);

        // selection outline
        extern RENDERING_SCENE_API void VisualizeSelectionOutline(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& targetColor, const ImageView& sceneDepth, const ImageView& selectionDepth, const FrameParams_SelectionOutline& params);

        //---------------------------------
        // debug visualizations

        // visualize depth buffer
        extern RENDERING_SCENE_API void VisualizeDepthBuffer(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& depthSource, const ImageView& targetColor);

        // visualize linearized depth buffer
        extern RENDERING_SCENE_API void VisualizeLinearDepthBuffer(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& depthSource, const ImageView& targetColor);

        // visualize raw luminance (no exposure)
        extern RENDERING_SCENE_API void VisualizeLuminance(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& colorSource, const ImageView& targetColor);

        // visualize a channel in the texture
        extern RENDERING_SCENE_API void VisualizeTexture(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& colorSource, const ImageView& targetColor, const base::Vector4& dot, const base::Vector4& mul = base::Vector4(1,1,1,1));

        //---------------------------------
        // light & shadows

        // bind the shadows data (cascades + shadow map atlas + terrain shadows)
        extern RENDERING_SCENE_API void BindShadowsData(command::CommandWriter& cmd, const CascadeData& cascades);

        // bind the lighting data (cascades + lighting grid)
        extern RENDERING_SCENE_API void BindLightingData(command::CommandWriter& cmd, const LightingData& lighting);

    } // scene
} // rendering

