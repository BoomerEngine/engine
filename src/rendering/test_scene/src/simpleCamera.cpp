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

BEGIN_BOOMER_NAMESPACE(rendering::test)

//---

SimpleCamera::SimpleCamera()
    : m_fov(90.0f)
{
    memset(m_keys, 0, sizeof(m_keys));
}

void SimpleCamera::placeCamera(const base::Vector3& position, const base::Angles& rotation)
{
    m_position = position;
    m_rotation = rotation;
}

void SimpleCamera::moveCamera(const base::Vector3& position, const base::Angles& rotation)
{
    m_position = position;
    m_rotation = rotation;
}

bool SimpleCamera::processInput(const base::input::BaseEvent& evt)
{
    // process events
    if (auto keyEvt  = evt.toKeyEvent())
    {
        auto state = keyEvt->pressed();

        switch (keyEvt->keyCode())
        {
            case base::input::KeyCode::KEY_W:
                m_keys[KEY_FOWARD] = state;
                return true;

            case base::input::KeyCode::KEY_S:
                m_keys[KEY_BACKWARD] = state;
                return true;

            case base::input::KeyCode::KEY_A:
                m_keys[KEY_LEFT] = state;
                return true;

            case base::input::KeyCode::KEY_D:
                m_keys[KEY_RIGHT] = state;
                return true;

            case base::input::KeyCode::KEY_Q:
                m_keys[KEY_UP] = state;
                return true;

            case base::input::KeyCode::KEY_E:
                m_keys[KEY_DOWN] = state;
                return true;

            case base::input::KeyCode::KEY_LEFT_SHIFT:
                m_keys[KEY_FAST] = state;
                return true;

            case base::input::KeyCode::KEY_LEFT_CTRL:
                m_keys[KEY_SLOW] = state;
                return true;
        }
    }
    else if (auto moveEvent  = evt.toMouseMoveEvent())
    {
        if (moveEvent->keyMask().isLeftDown())
        {
            m_rotation.pitch += moveEvent->delta().y * 0.1f;
            m_rotation.yaw += moveEvent->delta().x * 0.1f;
            return true;
        }
    }

    return false;
}

void SimpleCamera::update(float dt)
{
    base::Vector3 cameraMoveLocalDir(0, 0, 0);
    cameraMoveLocalDir.x += m_keys[KEY_FOWARD] ? 1.0f : 0.0f;
    cameraMoveLocalDir.x -= m_keys[KEY_BACKWARD] ? 1.0f : 0.0f;
    cameraMoveLocalDir.y += m_keys[KEY_RIGHT] ? 1.0f : 0.0f;
    cameraMoveLocalDir.y -= m_keys[KEY_LEFT] ? 1.0f : 0.0f;
    cameraMoveLocalDir.z += m_keys[KEY_UP] ? 1.0f : 0.0f;
    cameraMoveLocalDir.z -= m_keys[KEY_DOWN] ? 1.0f : 0.0f;

    float cameraSpeed = 5.0f;
    if (m_keys[KEY_FAST]) cameraSpeed = 10.0f;
    if (m_keys[KEY_SLOW]) cameraSpeed = 1.0f;

    if (!cameraMoveLocalDir.isZero())
    {
        cameraMoveLocalDir.normalize();
        cameraMoveLocalDir *= cameraSpeed;
    }

    base::Vector3 cameraDirForward, cameraDirRight, cameraDirUp;
    m_rotation.angleVectors(cameraDirForward, cameraDirRight, cameraDirUp);

    m_position += cameraDirForward * cameraMoveLocalDir.x * dt;
    m_position += cameraDirRight * cameraMoveLocalDir.y * dt;
    m_position += cameraDirUp * cameraMoveLocalDir.z * dt;
}

void SimpleCamera::calculateRenderData(scene::CameraSetup& outCamera) const
{ 
    outCamera.fov = m_fov;
    outCamera.position = m_position;
    outCamera.rotation = m_rotation.toQuat();
}

//---

END_BOOMER_NAMESPACE(rendering::test)
