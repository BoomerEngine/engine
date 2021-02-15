/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#include "build.h"
#include "gameFlyCamera.h"

#include "base/input/include/inputStructures.h"
#include "base/app/include/localServiceContainer.h"
#include "base/app/include/configProperty.h"
#include "base/ui/include/uiInputAction.h"
#include "base/ui/include/uiElement.h"

#include "rendering/scene/include/renderingFrameCamera.h"

namespace game
{
    //----

    namespace config
    {
        base::ConfigProperty<float> cvCameraNormalSpeed("Game.FlyCameraHelper", "CameraSpeed", 3.0f);
        base::ConfigProperty<float> cvCameraFastMultiplier("Game.FlyCameraHelper", "FastCameraSpeedMultiplier", 5.0f);
        base::ConfigProperty<float> cvCameraSlowMultiplier("Game.FlyCameraHelper", "SlowCameraSpeedMultiplier", 0.1f);
        base::ConfigProperty<float> cvCameraMaxAcceleration("Game.FlyCameraHelper", "MaxAcceleration", 50.0f);
        base::ConfigProperty<float> cvCameraMaxDeacceleration("Game.FlyCameraHelper", "MaxDeacceleration", 50.0f);
        base::ConfigProperty<float> cvCameraMouseSensitivity("Game.FlyCameraHelper", "MouseSensitivity", 0.2f);
    }

    //---

    enum FlyCameraHelperMovementKey
    {
        KEY_FOWARD,
        KEY_BACKWARD,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_UP,
        KEY_DOWN,
        KEY_FAST,
        KEY_SLOW,

        KEY_MAX,
    };

    FlyCameraHelperMovementKey MapMovementKey(const base::input::KeyCode keyCode)
    {
        switch (keyCode)
        {
            case base::input::KeyCode::KEY_W: return KEY_FOWARD;
            case base::input::KeyCode::KEY_S: return KEY_BACKWARD;
            case base::input::KeyCode::KEY_A: return KEY_LEFT;
            case base::input::KeyCode::KEY_D: return KEY_RIGHT;
            case base::input::KeyCode::KEY_Q: return KEY_UP;
            case base::input::KeyCode::KEY_E: return KEY_DOWN;
            case base::input::KeyCode::KEY_LEFT_CTRL: return KEY_SLOW;
            case base::input::KeyCode::KEY_LEFT_SHIFT: return KEY_FAST;
            case base::input::KeyCode::KEY_RIGHT_CTRL: return KEY_SLOW;
            case base::input::KeyCode::KEY_RIGHT_SHIFT: return KEY_FAST;
        }

        return FlyCameraHelperMovementKey::KEY_MAX;
    }

    //---

    FlyCameraHelper::FlyCameraHelper()
        : m_position(0,0,0)
        , m_rotation(0,0,0)
        , m_velocity(0,0,0)
        , m_speedFactor(1.0f)
    {
        resetInput();
    }

    void FlyCameraHelper::resetInput()
    {
        m_velocity = base::Vector3(0, 0, 0);
        memset(m_movementKeys, 0, sizeof(m_movementKeys));
        m_speedFactor = 1.0f;
    }

    void FlyCameraHelper::animate(float timeDelta)
    {
        computeMovement(timeDelta);
    }

    bool FlyCameraHelper::processKeyEvent(const base::input::KeyEvent& evt)
    {
        const auto key = MapMovementKey(evt.keyCode());
        if (key != FlyCameraHelperMovementKey::KEY_MAX)
        {
            if (evt.pressed())
                m_movementKeys[(int)key] = true;
            else if (evt.released())
                m_movementKeys[(int)key] = false;
            return true;
        }

        if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_NUMPAD_ADD)
        {
            m_speedFactor = std::min<float>(m_speedFactor * 1.1f, 100.0f);
            if (fabs(m_speedFactor - 1.0f) < 0.08f)
                m_speedFactor = 1.0f;
            return true;
        }
        else if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_NUMPAD_SUBTRACT)
        {
            m_speedFactor = std::max<float>(m_speedFactor / 1.1f, 0.1f);
            if (fabs(m_speedFactor - 1.0f) < 0.08f)
                m_speedFactor = 1.0f;
            return true;
        }

        return false;
    }

    void FlyCameraHelper::processMouseEvent(const base::input::MouseMovementEvent& evt)
    {
        base::Angles deltaRot(0, 0, 0);
        m_rotation.pitch = std::clamp<float>(m_rotation.pitch + evt.delta().y * config::cvCameraMouseSensitivity.get(), -90.0f, 90.0f);
        m_rotation.yaw += evt.delta().x * config::cvCameraMouseSensitivity.get();
    }

    void FlyCameraHelper::moveTo(const base::Vector3& position, const base::Angles& rotation)
    {
        m_velocity = base::Vector3(0, 0, 0);
        m_rotation = rotation;
        m_position = position;
    }

    void FlyCameraHelper::computeMovement(float timeDelta)
    {
        // speed
        float speed = 1.0f;
        if (m_movementKeys[(int)FlyCameraHelperMovementKey::KEY_FAST]) 
            speed *= config::cvCameraFastMultiplier.get();
        if (m_movementKeys[(int)FlyCameraHelperMovementKey::KEY_SLOW]) 
            speed /= config::cvCameraSlowMultiplier.get();


        // compute target velocity
        float cameraSpeed = config::cvCameraNormalSpeed.get() * pow(10.0f, m_speedFactor);
        base::Vector3 targetVelocity(0, 0, 0);
        {
            // aggregate movement
            base::Vector3 cameraMoveLocalDir(0, 0, 0);
            cameraMoveLocalDir.x += m_movementKeys[KEY_FOWARD] ? 1.0f : 0.0f;
            cameraMoveLocalDir.x -= m_movementKeys[KEY_BACKWARD] ? 1.0f : 0.0f;
            cameraMoveLocalDir.y += m_movementKeys[KEY_RIGHT] ? 1.0f : 0.0f;
            cameraMoveLocalDir.y -= m_movementKeys[KEY_LEFT] ? 1.0f : 0.0f;
            cameraMoveLocalDir.z += m_movementKeys[KEY_UP] ? 1.0f : 0.0f;
            cameraMoveLocalDir.z -= m_movementKeys[KEY_DOWN] ? 1.0f : 0.0f;
            if (!cameraMoveLocalDir.isZero())
            {
                // prevent faster than max movement when both strafing and moving forward
                cameraMoveLocalDir.normalize();

                // compute 3D velocity
                base::Vector3 cameraDirForward, cameraDirRight, cameraDirUp;
                m_rotation.angleVectors(cameraDirForward, cameraDirRight, cameraDirUp);

                targetVelocity += cameraDirForward * cameraMoveLocalDir.x;
                targetVelocity += cameraDirRight * cameraMoveLocalDir.y;
                targetVelocity += cameraDirUp * cameraMoveLocalDir.z;
                targetVelocity *= cameraSpeed;
            }
        }

        // accelerate towards target velocity
        {
            // calculate required velocity change
            auto delta = targetVelocity - m_velocity;

            // limit the maximum velocity change
            auto acc = targetVelocity.isZero() ? config::cvCameraMaxDeacceleration.get() : config::cvCameraMaxAcceleration.get();
            m_velocity += base::ClampLength(delta, acc * timeDelta * (cameraSpeed / config::cvCameraNormalSpeed.get()));
        }

        // move the camera
        auto deltaPos = m_velocity * timeDelta;
        m_position += deltaPos;
    }
    
    void FlyCameraHelper::computeRenderingCamera(rendering::scene::CameraSetup& outCamera) const
    {
        outCamera.position = m_position;
        outCamera.rotation = m_rotation.toQuat();
        outCamera.nearPlane = 0.05f;
        outCamera.farPlane = 8000.0f;
        outCamera.zoom = 1.0f;
    }

    //---

} // ui
