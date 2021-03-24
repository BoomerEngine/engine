/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#include "build.h"
#include "entityFreeCamera.h"

#include "engine/world/include/world.h"
#include "engine/canvas/include/canvas.h"

#include "core/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(FreeCameraEntity);
RTTI_END_TYPE();

//--

ConfigProperty<float> cvCameraNormalSpeed("Game.FreeCamera", "CameraSpeed", 5.0f);
ConfigProperty<float> cvCameraFastMultiplier("Game.FreeCamera", "FastCameraSpeedMultiplier", 2.0f);
ConfigProperty<float> cvCameraSlowMultiplier("Game.FreeCamera", "SlowCameraSpeedMultiplier", 0.2f);
ConfigProperty<float> cvCameraMaxAcceleration("Game.FreeCamera", "MaxAcceleration", 50.0f);
ConfigProperty<float> cvCameraMaxDeacceleration("Game.FreeCamera", "MaxDeacceleration", 50.0f);
ConfigProperty<float> cvCameraMouseSensitivity("Game.FreeCamera", "MouseSensitivity", 0.2f);

//--

FreeCamera::FreeCamera()
{

}

void FreeCamera::reset()
{
    m_buttons = ButtonMask();
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;
}

bool FreeCamera::input(const InputEvent& evt)
{
    if (const auto* key = evt.toKeyEvent())
    {
        const auto value = key->isDown();

        switch (key->keyCode())
        {
            case InputKey::KEY_A: m_buttons.configure(boomer::FreeCamera::ButtonBit::Left, value); return true;
            case InputKey::KEY_D: m_buttons.configure(boomer::FreeCamera::ButtonBit::Right, value); return true;
            case InputKey::KEY_W: m_buttons.configure(boomer::FreeCamera::ButtonBit::Forward, value);; return true;
            case InputKey::KEY_S: m_buttons.configure(boomer::FreeCamera::ButtonBit::Backward, value);; return true;
            case InputKey::KEY_Q: m_buttons.configure(boomer::FreeCamera::ButtonBit::Down, value);; return true;
            case InputKey::KEY_E: m_buttons.configure(boomer::FreeCamera::ButtonBit::Up, value);; return true;
            case InputKey::KEY_LEFT_SHIFT: m_buttons.configure(boomer::FreeCamera::ButtonBit::Fast, value);; return true;
            case InputKey::KEY_LEFT_CTRL: m_buttons.configure(boomer::FreeCamera::ButtonBit::Slow, value);; return true;
        }
    }
    else if (const auto* axis = evt.toAxisEvent())
    {
        switch (axis->axisCode())
        {
            case InputAxis::AXIS_MOUSEX: m_mouseDeltaX = axis->displacement() * cvCameraMouseSensitivity.get(); return true;
            case InputAxis::AXIS_MOUSEY: m_mouseDeltaY = axis->displacement() * cvCameraMouseSensitivity.get(); return true;
        }
    }

    return false;
}

void FreeCamera::update(float dt, Vector3& outDeltaPosition)
{
    Vector3 cameraMoveLocalDir(0, 0, 0);
    cameraMoveLocalDir.x += m_buttons.test(boomer::FreeCamera::ButtonBit::Forward) ? 1.0f : 0.0f;
    cameraMoveLocalDir.x += m_buttons.test(boomer::FreeCamera::ButtonBit::Backward) ? -1.0f : 0.0f;
    cameraMoveLocalDir.y += m_buttons.test(boomer::FreeCamera::ButtonBit::Left) ? -1.0f : 0.0f;
    cameraMoveLocalDir.y += m_buttons.test(boomer::FreeCamera::ButtonBit::Right) ? 1.0f : 0.0f;
    cameraMoveLocalDir.z += m_buttons.test(boomer::FreeCamera::ButtonBit::Up) ? 1.0f : 0.0f;
    cameraMoveLocalDir.z += m_buttons.test(boomer::FreeCamera::ButtonBit::Down) ? -1.0f : 0.0f;

    float cameraSpeed = cvCameraNormalSpeed.get();
    if (m_buttons.test(boomer::FreeCamera::ButtonBit::Fast)) cameraSpeed *= cvCameraFastMultiplier.get();
    if (m_buttons.test(boomer::FreeCamera::ButtonBit::Slow)) cameraSpeed *= cvCameraSlowMultiplier.get();

    if (!cameraMoveLocalDir.isZero())
    {
        cameraMoveLocalDir.normalize();
        cameraMoveLocalDir *= cameraSpeed;
    }

    Vector3 cameraDirForward, cameraDirRight, cameraDirUp;
    m_rotation.angleVectors(cameraDirForward, cameraDirRight, cameraDirUp);

    Vector3 deltaPosition(0, 0, 0);
    deltaPosition += cameraDirForward * cameraMoveLocalDir.x * dt;
    deltaPosition += cameraDirRight * cameraMoveLocalDir.y * dt;
    deltaPosition += cameraDirUp * cameraMoveLocalDir.z * dt;

    m_rotation.pitch = std::clamp<float>(m_rotation.pitch + m_mouseDeltaY, -90.0f, 90.0f);
    m_rotation.yaw += m_mouseDeltaX;
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;

    outDeltaPosition = deltaPosition;


    /*
     // accelerate towards target velocity
{
    // calculate required velocity change
    auto delta = targetVelocity - m_velocity;

    // limit the maximum velocity change
    auto acc = targetVelocity.isZero() ? config::cvCameraMaxDeacceleration.get() : config::cvCameraMaxAcceleration.get();
    m_velocity += ClampLength(delta, acc * timeDelta * (cameraSpeed / config::cvCameraNormalSpeed.get()));
}
*/

}

//--

FreeCameraEntity::FreeCameraEntity()
{
    if (!IsDefaultObjectCreation())
        m_observer = RefNew<WorldPersistentObserver>();
}

FreeCameraEntity::~FreeCameraEntity()
{}

bool FreeCameraEntity::processRawInput(const InputEvent& evt)
{
    if (m_freeCamera.input(evt))
        return true;

    return false;
}

void FreeCameraEntity::place(const ExactPosition& pos)
{
    Transform transform;
    transform.T = pos;
    transform.R = m_freeCamera.m_rotation.toQuat();
    requestTransformChangeWorldSpace(transform);
}

void FreeCameraEntity::place(const ExactPosition& pos, const Angles& rot)
{
    m_freeCamera.m_rotation = rot;

    Transform transform;
    transform.T = pos;
    transform.R = m_freeCamera.m_rotation.toQuat();
    requestTransformChangeWorldSpace(transform);
}

void FreeCameraEntity::handleAttach()
{
    TBaseClass::handleAttach();
    world()->attachPersistentObserver(m_observer);
}

void FreeCameraEntity::handleDetach()
{
    TBaseClass::handleDetach();
    world()->dettachPersistentObserver(m_observer);
}

void FreeCameraEntity::handleUpdateMask(WorldUpdateMask& outUpdateMask) const
{
    outUpdateMask |= WorldUpdatePhase::PreTick;
}

void FreeCameraEntity::renderDebugCanvas(Canvas& c) const
{
    const auto pos = cachedWorldTransform().T;

    c.debugPrint(c.width() - 20, c.height() - 60,
        TempString("FC location: [X={}, Y={}, Z={}, Pitch={}, Yaw={}]",
            Prec(pos.x, 2), Prec(pos.y, 2), Prec(pos.z, 2),
            Prec(m_freeCamera.m_rotation.pitch, 1), Prec(m_freeCamera.m_rotation.yaw, 1)),
        Color::WHITE, 16, font::FontAlignmentHorizontal::Right);

    c.debugPrint(c.width() - 20, c.height() - 40,
        TempString("FC streaming: {}", m_modeUpdateStreaming),
        m_modeUpdateStreaming ? Color::WHITE : Color::RED, 16, font::FontAlignmentHorizontal::Right);
}

void FreeCameraEntity::handleUpdate(const EntityThreadContext& tc, WorldUpdatePhase phase, float dt)
{
    if (phase == WorldUpdatePhase::PreTick)
    {
        Vector3 deltaPosition;
        m_freeCamera.update(dt, deltaPosition);

        Transform transform;
        transform.T = cachedWorldTransform().T + deltaPosition;
        transform.R = m_freeCamera.m_rotation.toQuat();
        requestTransformChangeWorldSpace(transform);

        if (m_modeUpdateStreaming)
            m_observer->move(transform.T);
    }
}

//--

END_BOOMER_NAMESPACE()
