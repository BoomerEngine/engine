/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"

namespace rendering
{
    namespace scene
    {
        //---

        /// view rendering helper
        class RENDERING_SCENE_API FrameView : public base::NoCopy
        {
        public:
            FrameView(const FrameRenderer& frame, const FrameViewCamera& camera, base::StringView<char> name);
            ~FrameView();

            //--

            // prepare view for rendering - collect all heavy duty objects, thread safe - should modify only the data inside the view
            void collect(); 

            // generate rendering fragments from collected visuble stuff (for actual rendering)
            // NOTE: this may generate some additional rendering commands based on the collected content
            void generateFragments(command::CommandWriter& cmd);

            //--

            struct PerSceneData
            {
                const Scene* scene = nullptr;
                ParametersView params;
                SceneObjectCullingResult collectedObjects;
                FragmentDrawList* drawList = nullptr;
            };

            // get per-scene data, we are mostly interested in draw list and list of collected proxies
            INLINE const base::Array<PerSceneData*>& scenes() const { return m_scenes; }

            // parent frame
            INLINE const FrameParams& frame() const { return m_frame; }

        private:
            const FrameRenderer& m_renderer;
            const FrameParams& m_frame;
            const FrameViewCamera& m_camera;
            base::mem::LinearAllocator m_allocator;
            base::InplaceArray<PerSceneData*, 10> m_scenes;

            base::StringBuf m_name;
        };

        //--

        // render a LIT scene into a linear color buffer, does MSAA resolve if MSAA mode was used but NO post processing - the result is a set of TWO Linear HDR buffers with scene content
        extern RENDERING_SCENE_API void RenderLitView(command::CommandWriter& cmd, const FrameRenderer& frame, const FrameViewCamera& camera, const ImageView& resolvedColor, const ImageView& resolvedDepth);

        // render a solid wireframe view
        extern RENDERING_SCENE_API void RenderWireframeView(command::CommandWriter& cmd, const FrameRenderer& frame, const FrameViewCamera& camera, const ImageView& resolvedColor, bool solid);

        //--

        // render a depth buffer debug
        extern RENDERING_SCENE_API void RenderDepthDebug(command::CommandWriter& cmd, const FrameRenderer& frame, const FrameViewCamera& camera, const ImageView& resolvedColor);

        // render a luminance debug
        extern RENDERING_SCENE_API void RenderLuminanceDebug(command::CommandWriter& cmd, const FrameRenderer& frame, const FrameViewCamera& camera, const ImageView& resolvedColor);

        //--

    } // scene
} // rendering

