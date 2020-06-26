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

        // a "bracket" (collection of states) for active pass (PASS = bound render targets)
        struct PassBracket
        {
        public:
            PassBracket(command::CommandWriter& cmd, const FrameView& view, base::StringView<char> name);
            ~PassBracket();

            // bind a depth buffer and clear it
            void depthClear(const ImageView& rt);

            // bind a color buffer and clear it
            void colorClear(uint8_t index, const ImageView& rt, const base::Vector4& clearValues = base::Vector4(0, 0, 0, 1));

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

        // render depth pre pass
        extern RENDERING_SCENE_API void RenderDepthPrepass(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const ImageView& depthRT);

        // render forward pass
        extern RENDERING_SCENE_API void RenderForwardPass(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const ImageView& depthRT, const ImageView& colorRT);

        // render wireframe pass
        extern RENDERING_SCENE_API void RenderWireframePass(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const ImageView& depthRT, const ImageView& colorRT, bool solid);

        //--

    } // scene
} // rendering

