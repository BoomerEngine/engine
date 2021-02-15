/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: camera #]
***/

#include "build.h"
#include "base/input/include/inputStructures.h"
#include "renderingSimpleFlyCamera.h"
#include "renderingFrameParams.h"
#include "renderingFrameCameraContext.h"

namespace rendering
{
    namespace scene
    {
        //---

        FlyCamera::FlyCamera()
        {
            m_context = base::RefNew<CameraContext>();
        }

        FlyCamera::~FlyCamera()
        {}

        bool FlyCamera::processRawInput(const base::input::BaseEvent& evt)
        {
            if (const auto* key = evt.toKeyEvent())
            {
                if (key->pressed() || key->released())
                {
                    float value = key->pressed() ? 1.0f : -1.0f;
                    switch (key->keyCode())
                    {
                        case base::input::KeyCode::KEY_A: m_buttonLeftRight -= value; return true;
                        case base::input::KeyCode::KEY_D: m_buttonLeftRight += value; return true;
                        case base::input::KeyCode::KEY_W: m_buttonForwardBackward += value; return true;
                        case base::input::KeyCode::KEY_S: m_buttonForwardBackward -= value; return true;
                        case base::input::KeyCode::KEY_Q: m_buttonUpDown -= value; return true;
                        case base::input::KeyCode::KEY_E: m_buttonUpDown += value; return true;
                    }
                }
            }
            else if (const auto* axis = evt.toAxisEvent())
            {
                switch (axis->axisCode())
                {
                    case base::input::AxisCode::AXIS_MOUSEX: m_mouseDeltaX = axis->displacement() * 0.25f; return true;
                    case base::input::AxisCode::AXIS_MOUSEY: m_mouseDeltaY = axis->displacement() * 0.25f; return true;
                }                
            }

            return false;
        }

        void FlyCamera::compute(rendering::scene::FrameParams& params) const
        {
            CameraSetup cameraParams;
            cameraParams.position = m_position;
            cameraParams.rotation = m_rotation.toQuat();
            cameraParams.fov = 75.0f;
            cameraParams.aspect = params.resolution.aspect();
            cameraParams.nearPlane = 0.01f;
            cameraParams.farPlane = 500.0f;

            params.camera.camera.setup(cameraParams);
            params.camera.cameraContext = m_context;
        }

        void FlyCamera::place(const base::Vector3& initialPos, const base::Angles& initialRotation)
        {
            m_position = initialPos;
            m_rotation = initialRotation;
        }


        void FlyCamera::update(float dt)
        {
            base::Vector3 cameraMoveLocalDir(0, 0, 0);
            cameraMoveLocalDir.x = m_buttonForwardBackward;
            cameraMoveLocalDir.y = m_buttonLeftRight;
            cameraMoveLocalDir.z = m_buttonUpDown;

            float cameraSpeed = 5.0f;
            //if (m_keys[KEY_FAST]) cameraSpeed = 10.0f;
            //if (m_keys[KEY_SLOW]) cameraSpeed = 1.0f;

            if (!cameraMoveLocalDir.isZero())
            {
                cameraMoveLocalDir.normalize();
                cameraMoveLocalDir *= cameraSpeed;
            }

            base::Vector3 cameraDirForward, cameraDirRight, cameraDirUp;
            m_rotation.angleVectors(cameraDirForward, cameraDirRight, cameraDirUp);

            base::Vector3 deltaPosition(0,0,0);
            deltaPosition += cameraDirForward * cameraMoveLocalDir.x * dt;
            deltaPosition += cameraDirRight * cameraMoveLocalDir.y * dt;
            deltaPosition += cameraDirUp * cameraMoveLocalDir.z * dt;

            m_rotation.pitch = std::clamp<float>(m_rotation.pitch + m_mouseDeltaY, -90.0f, 90.0f);
            m_rotation.yaw += m_mouseDeltaX;
            m_mouseDeltaX = 0.0f;
            m_mouseDeltaY = 0.0f;

            m_position += deltaPosition;
        }

        //---

    } // test
} // rendering
