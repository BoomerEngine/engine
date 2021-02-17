/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#pragma once

namespace game
{

    //---

    /// simple (debug) camera that can fly around the level
    class GAME_HOST_API FreeCameraHelper : public base::NoCopy
    {
    public:
        FreeCameraHelper();

        ///--

        // get current (animated) camera position
        INLINE const base::Vector3& position() const { return m_position; }

        // get current (animated) camera rotation
        INLINE const base::Angles& rotation() const { return m_rotation; }

        ///--

        /// reset input state of the controller
        void resetInput();

        /// update camera animation/handling
        void animate(float timeDelta);

        /// move camera to given place
        void moveTo(const base::Vector3& position, const base::Angles& rotation);

        /// process key event, moves the camera around
        bool processInput(const base::input::BaseEvent& evt);

        //--

        // compute camera settings
        void computeRenderingCamera(rendering::scene::CameraSetup& outCamera) const;

    private:
        base::Vector3 m_velocity;
        base::Vector3 m_position;
        base::Angles m_rotation;

        bool m_movementKeys[8];

        float m_speedFactor;

        void computeMovement(float timeDelta);

        bool processKeyEvent(const base::input::KeyEvent& evt);
        bool processMouseEvent(const base::input::AxisEvent& evt);
    };

    //---

} // ui