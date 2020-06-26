/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"
#include "renderingSceneUtils.h"

namespace rendering
{
    namespace scene
    {

        //---

        /// render camera
        struct RENDERING_SCENE_API FrameViewCameraData
        {
            GPUCameraInfo data;

            // TODO: more ?
        };

        //---

        /// view camera
        class RENDERING_SCENE_API FrameViewCamera : public base::NoCopy
        {
        public:
            FrameViewCamera(const FrameParams& params);

            ///--

            // main FRAME camera, always present
            INLINE const Camera& mainCamera() const { return m_mainCamera; }

            // render cameras, we can have multiple per view, they can also be different per view
            INLINE const base::Array<FrameViewCameraData>& renderCameras() const { return m_renderCameras; }

            ///--

            // write camera setup into the recorded command buffer
            void bind(command::CommandWriter& cmd) const;

            ///---

            /// generate main camera setup - creates ONE or TWO render cameras - depending if we are VR or not, cameras will include Jittered matrices if TAA is used
            void calcMainCamera();

            //--

        private:
            const FrameParams& m_params;

            const Camera& m_mainCamera;
            base::Array<FrameViewCameraData> m_renderCameras;
        };

        //---

    } // scene
} // rendering

