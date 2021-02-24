/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "gameFreeCameraHelper.h"

#include "base/input/include/inputStructures.h"
#include "base/app/include/localServiceContainer.h"
#include "base/app/include/configProperty.h"

#include "rendering/scene/include/renderingFrameCamera.h"

BEGIN_BOOMER_NAMESPACE(game)

//----

namespace config
{
    base::ConfigProperty<float> cvCameraNormalSpeed("Game.FreeCameraHelper", "CameraSpeed", 3.0f);
    base::ConfigProperty<float> cvCameraFastMultiplier("Game.FreeCameraHelper", "FastCameraSpeedMultiplier", 5.0f);
    base::ConfigProperty<float> cvCameraSlowMultiplier("Game.FreeCameraHelper", "SlowCameraSpeedMultiplier", 0.1f);
    base::ConfigProperty<float> cvCameraMaxAcceleration("Game.FreeCameraHelper", "MaxAcceleration", 50.0f);
    base::ConfigProperty<float> cvCameraMaxDeacceleration("Game.FreeCameraHelper", "MaxDeacceleration", 50.0f);
    base::ConfigProperty<float> cvCameraMouseSensitivity("Game.FreeCameraHelper", "MouseSensitivity", 0.2f);
}

//---

enum FreeCameraHelperMovementKey
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

FreeCameraHelperMovementKey MapMovementKey(const base::input::KeyCode keyCode)
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

    return FreeCameraHelperMovementKey::KEY_MAX;
}

//---

FreeCameraHelper::FreeCameraHelper()
    : m_position(0,0,0)
    , m_rotation(0,0,0)
    , m_velocity(0,0,0)
    , m_speedFactor(1.0f)
{
    resetInput();
}

void FreeCameraHelper::resetInput()
{
    m_velocity = base::Vector3(0, 0, 0);
    memset(m_movementKeys, 0, sizeof(m_movementKeys));
    m_speedFactor = 1.0f;
}

void FreeCameraHelper::animate(float timeDelta)
{
    computeMovement(timeDelta);
}

bool FreeCameraHelper::processInput(const base::input::BaseEvent& evt)
{
    if (const auto* keyEvent = evt.toKeyEvent())
        return processKeyEvent(*keyEvent);
    else if (const auto* mouseEvent = evt.toAxisEvent())
        return processMouseEvent(*mouseEvent);

    return false;
}

bool FreeCameraHelper::processKeyEvent(const base::input::KeyEvent& evt)
{
    const auto key = MapMovementKey(evt.keyCode());
    if (key != FreeCameraHelperMovementKey::KEY_MAX)
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

bool FreeCameraHelper::processMouseEvent(const base::input::AxisEvent& evt)
{
    if (evt.axisCode() == base::input::AxisCode::AXIS_MOUSEX)
        m_rotation.yaw += evt.displacement() * config::cvCameraMouseSensitivity.get();
    else if (evt.axisCode() == base::input::AxisCode::AXIS_MOUSEY)
        m_rotation.pitch = std::clamp<float>(m_rotation.pitch + evt.displacement() * config::cvCameraMouseSensitivity.get(), -90.0f, 90.0f);

    return true;
}

void FreeCameraHelper::moveTo(const base::Vector3& position, const base::Angles& rotation)
{
    m_velocity = base::Vector3(0, 0, 0);
    m_rotation = rotation;
    m_position = position;
}

void FreeCameraHelper::computeMovement(float timeDelta)
{
    // speed
    float speed = 1.0f;
    if (m_movementKeys[(int)FreeCameraHelperMovementKey::KEY_FAST]) 
        speed *= config::cvCameraFastMultiplier.get();
    if (m_movementKeys[(int)FreeCameraHelperMovementKey::KEY_SLOW]) 
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
    
void FreeCameraHelper::computeRenderingCamera(rendering::scene::CameraSetup& outCamera) const
{
    outCamera.position = m_position;
    outCamera.rotation = m_rotation.toQuat();
    outCamera.nearPlane = 0.05f;
    outCamera.farPlane = 8000.0f;
    outCamera.zoom = 1.0f;
}

//---

END_BOOMER_NAMESPACE(game)
