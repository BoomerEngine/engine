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

        class RENDERING_SCENE_API FrameView_Selection : public FrameView
        {
        public:
            FrameView_Selection(const FrameRenderer& frame, const Camera& camera, ImageView depthTarget, const base::Rect& captureRect, const DownloadBufferPtr& captureData);

            void render(command::CommandWriter& cmd);

        private:
            const Camera& m_camera;
            
            ImageView m_depthTarget;
            
            base::Rect m_captureRect;
            DownloadBufferPtr m_captureData;
        };

        //--

    } // scene
} // rendering

