/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

/// simple (debug) camera that can fly around the level
class ENGINE_GAME_API FreeCameraHelper : public NoCopy
{
public:
    FreeCameraHelper();

    ///--

    // get current (animated) camera position
    INLINE const Vector3& position() const { return m_position; }

    // get current (animated) camera rotation
    INLINE const Angles& rotation() const { return m_rotation; }

    ///--

    /// reset input state of the controller
    void resetInput();

    /// update camera animation/handling
    void animate(float timeDelta);

    /// move camera to given place
    void moveTo(const Vector3& position, const Angles& rotation);

    /// process key event, moves the camera around
    bool processInput(const input::BaseEvent& evt);

    //--

    // compute camera settings
    void computeRenderingCamera(CameraSetup& outCamera) const;

private:
    Vector3 m_velocity;
    Vector3 m_position;
    Angles m_rotation;

    bool m_movementKeys[8];

    float m_speedFactor;

    void computeMovement(float timeDelta);

    bool processKeyEvent(const input::KeyEvent& evt);
    bool processMouseEvent(const input::AxisEvent& evt);
};

//---

END_BOOMER_NAMESPACE()
