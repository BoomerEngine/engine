/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#pragma once

#include "entityCamera.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// free camera controller - entity-less code to service free camera
class ENGINE_WORLD_ENTITIES_API FreeCamera : public NoCopy
{
public:
    FreeCamera();

    Angles m_rotation;

    void reset();
    bool input(const InputEvent& evt);
    void update(float dt, Vector3& outDeltaPosition);

private:
    enum class ButtonBit : uint16_t
    {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down,
        Fast,
        Slow,
    };

    typedef BitFlags<ButtonBit> ButtonMask;

    ButtonMask m_buttons;

    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
};

//--

// a scene component that is usable as a camera
class ENGINE_WORLD_ENTITIES_API FreeCameraEntity : public CameraEntity
{
    RTTI_DECLARE_VIRTUAL_CLASS(FreeCameraEntity, CameraEntity);

public:
    FreeCameraEntity();
    virtual ~FreeCameraEntity();

    ///---

    // move camera to new position
    void place(const ExactPosition& pos);

    // move camera to new position and rotation
    void place(const ExactPosition& pos, const Angles& rot);

    //--

    // process raw input to fly the camera (free camera is not mapped)
    bool processRawInput(const InputEvent& evt);

    // draw on-screen debug information
    void renderDebugCanvas(canvas::Canvas& c) const;

    //--

protected:
    virtual void handleUpdate(const EntityThreadContext& tc, WorldUpdatePhase phase, float dt) override final;
    virtual void handleUpdateMask(WorldUpdateMask& outUpdateMask) const override final;
    virtual void handleAttach() override final;
    virtual void handleDetach() override final;

private:
    FreeCamera m_freeCamera;

    bool m_modeUpdateStreaming = true;

    WorldPersistentObserverPtr m_observer;
};

//--

END_BOOMER_NAMESPACE()
