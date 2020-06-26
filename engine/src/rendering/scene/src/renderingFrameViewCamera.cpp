/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneUtils.h"
#include "renderingFrameViewCamera.h"

namespace rendering
{
    namespace scene
    {
        //---

        FrameViewCamera::FrameViewCamera(const FrameParams& params)
            : m_params(params)
            , m_mainCamera(params.camera.camera)
        {
        }

        void FrameViewCamera::bind(command::CommandWriter& cmd) const
        {
            DEBUG_CHECK(!m_renderCameras.empty());

            struct
            {
                ConstantsView params;
            }  desc;

            desc.params = cmd.opUploadConstants(m_renderCameras[0].data);
            cmd.opBindParametersInline("CameraParams"_id, desc);

        }

        void FrameViewCamera::calcMainCamera()
        {
            DEBUG_CHECK(m_renderCameras.empty());
            m_renderCameras.reset();

            // create main render camera
            auto& renderCamera = m_renderCameras.emplaceBack();
            CalcGPUCameraInfo(renderCamera.data, m_mainCamera);
        }

        //---

    } // scene
} // rendering


