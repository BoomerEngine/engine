/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameView.h"

namespace rendering
{
    namespace scene
    {

        //--

        class RENDERING_SCENE_API FrameView_Main : public FrameView
        {
        public:
            FrameView_Main(const FrameRenderer& frame, const Camera& camera, ImageView colorTarget, ImageView depthTarget, FrameRenderMode mode = FrameRenderMode::Default);

            void render(command::CommandWriter& cmd);

        private:
            const Camera& m_camera;
            
            ImageView m_colorTarget;
            ImageView m_depthTarget;
            FrameRenderMode m_mode;
        };

        //--

    } // scene
} // rendering

