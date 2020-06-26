/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: viewer #]
***/

#include "build.h"
#include "simpleCamera.h"

namespace viewer
{
    //---

    SimpleCamera::SimpleCamera()
        : m_fov(90.0f)
    {
        memset(m_keys, 0, sizeof(m_keys));
    }

    void SimpleCamera::placeCamera(const Vector3& position, const Angles& rotation)
    {
        m_position = position;
        m_rotation = rotation;
    }

    void SimpleCamera::moveCamera(const Vector3& position, const Angles& rotation)
    {
        m_position = position;
        m_rotation = rotation;
    }

    bool SimpleCamera::processInput(const BaseEvent& evt)
    {
        // process events
        if (auto keyEvt  = evt.toKeyEvent())
        {
            auto state = keyEvt->pressed();

            switch (keyEvt->keyCode())
            {
                case KeyCode::KEY_W:
                    m_keys[KEY_FOWARD] = state;
                    return true;

                case KeyCode::KEY_S:
                    m_keys[KEY_BACKWARD] = state;
                    return true;

                case KeyCode::KEY_A:
                    m_keys[KEY_LEFT] = state;
                    return true;

                case KeyCode::KEY_D:
                    m_keys[KEY_RIGHT] = state;
                    return true;

                case KeyCode::KEY_Q:
                    m_keys[KEY_UP] = state;
                    return true;

                case KeyCode::KEY_E:
                    m_keys[KEY_DOWN] = state;
                    return true;

                case KeyCode::KEY_LEFT_SHIFT:
                    m_keys[KEY_FAST] = state;
                    return true;

                case KeyCode::KEY_LEFT_CTRL:
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
        Vector3 cameraMoveLocalDir(0, 0, 0);
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

        Vector3 cameraDirForward, cameraDirRight, cameraDirUp;
        m_rotation.angleVectors(cameraDirForward, cameraDirRight, cameraDirUp);

        m_position += cameraDirForward * cameraMoveLocalDir.x * dt;
        m_position += cameraDirRight * cameraMoveLocalDir.y * dt;
        m_position += cameraDirUp * cameraMoveLocalDir.z * dt;
    }

    void SimpleCamera::calculateRenderData(uint32_t width, uint32_t height, SimpleCameraState& outCamera) const
    { 
        float aspect = height ? width / (float)height : 1.0f;
        float zoom = 1.0f;

        static Matrix CameraToView(Vector4(0, 1, 0, 0), Vector4(0, 0, -1, 0), Vector4(1, 0, 0, 0), Vector4(0, 0, 0, 1));
        static const Matrix ViewToCamera = CameraToView.inverted();

        outCamera.WorldToCamera = m_rotation.toMatrixTransposed();
        outCamera.WorldToCamera.translation(-outCamera.WorldToCamera.transformVector(m_position));

        if (m_fov == 0.0f)
            outCamera.ViewToScreen = Matrix::BuildOrtho(zoom * aspect, zoom, -50.0f, 50.0f);
        else
            outCamera.ViewToScreen = Matrix::BuildPerspectiveFOV(m_fov, aspect, 0.01f, 2000.0f);

        outCamera.ScreenToView = outCamera.ViewToScreen.inverted();
        outCamera.CameraToScreen = CameraToView * outCamera.ViewToScreen;

        outCamera.WorldToScreen = outCamera.WorldToCamera * outCamera.CameraToScreen;
        outCamera.ScreenToWorld = outCamera.WorldToScreen.inverted();
    }

    //---

} // viewer
