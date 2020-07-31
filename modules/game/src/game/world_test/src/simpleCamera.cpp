/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: camera #]
***/

#include "build.h"
#include "base/input/include/inputStructures.h"
#include "rendering/scene/include/renderingFrameCamera.h"
#include "simpleCamera.h"

#include "game/host/include/gameInputMapping.h"
#include "game/host/include/gameInputContext.h"
#include "game/world/include/worldInputComponent.h"
#include "game/world/include/worldCameraComponent.h"

namespace game
{
    namespace test
    {
        //---

        base::res::StaticResource<InputDefinitions> resFreeCameraInput("/game/input/free_camera.v4input");

        //---

        RTTI_BEGIN_TYPE_CLASS(FlyCameraEntity);
        RTTI_END_TYPE();

        FlyCameraEntity::FlyCameraEntity()
        {
            m_camera = base::CreateSharedPtr<CameraComponent>();
            attachComponent(m_camera);

            m_input = base::CreateSharedPtr<InputComponent>(resFreeCameraInput.loadAndGet());
            attachComponent(m_input);
        }

        FlyCameraEntity::~FlyCameraEntity()
        {}

        void FlyCameraEntity::place(const base::Vector3& initialPos, const base::Angles& initialRotation)
        {
            m_rotation = initialRotation;
            requestMove(initialPos, initialRotation.toQuat());
        }


        void FlyCameraEntity::updateCamera(float dt)
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

            const auto oldRot = m_rotation;
            m_rotation.pitch = std::clamp<float>(m_rotation.pitch + m_mouseDeltaY, -90.0f, 90.0f);
            m_rotation.yaw += m_mouseDeltaX;
            m_mouseDeltaX = 0.0f;
            m_mouseDeltaY = 0.0f;

            if (!deltaPosition.isZero() || oldRot != m_rotation)
            {
                const auto newPos = absoluteTransform().position() + deltaPosition;
                requestMove(newPos, m_rotation.toQuat());
            }
        }

        void FlyCameraEntity::handlePreTick(float dt)
        {
            TBaseClass::handlePreTick(dt);
            updateCamera(dt);
        }

        void FlyCameraEntity::handleGameInput(const InputEventPtr& gameInput)
        {
            if (gameInput->m_name == "FreeCameraFB"_id)
            {
                m_buttonForwardBackward = gameInput->m_absoluteValue;
            }
            else if (gameInput->m_name == "FreeCameraLR"_id)
            {
                m_buttonLeftRight = gameInput->m_absoluteValue;
            }
            else if (gameInput->m_name == "FreeCameraUD"_id)
            {
                m_buttonUpDown = gameInput->m_absoluteValue;
            }
            else if (gameInput->m_name == "FreeCameraX"_id)
            {
                m_mouseDeltaX = gameInput->m_deltaValue * 0.25f;
            }
            else if (gameInput->m_name == "FreeCameraY"_id)
            {
                m_mouseDeltaY = gameInput->m_deltaValue * 0.25f;
            }
        }

        void FlyCameraEntity::activate()
        {
            m_input->activate();
            m_camera->activate();
        }

        void FlyCameraEntity::deactivate()
        {
            m_input->deactivate();
            m_camera->deactivate();
        }


        //---

    } // test
} // rendering
