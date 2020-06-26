/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#pragma once

namespace ui
{

    /// simple perspective rendering camera controller
    class RENDERING_UI_API CameraController
    {
    public:
        CameraController();

        ///--

        // get current (animated) camera position
        INLINE const base::Vector3& position() const { return m_currentPosition; }

        // get current (animated) camera rotation
        INLINE const base::Angles& rotation() const { return m_currentRotation; }

        // get current speed multiplier
        INLINE float speedFactor() const { return m_speedFactor; }

        // get current camera stamp, changed every time camera moves
        INLINE uint32_t stamp() const { return m_stamp; }

        // get current distance to origin
        INLINE float currentDistanceToOrigin() const { return m_currentPosition.length(); }

        ///--

        /// reset input state of the controller
        void resetInput();

        /// update camera animation/handling
        void animate(float timeDelta);

        /// move camera to given place
        void moveTo(const base::Vector3& position, const base::Angles& rotation);

        ///---

        /// process key event, moves the camera around
        bool processKeyEvent(const base::input::KeyEvent& evt);

        /// process mouse wheel
        void processMouseWheel(const base::input::MouseMovementEvent& evt, float delta);

        /// process mouse event
        InputActionPtr handleGeneralFly(IElement* owner, uint8_t button);

        /// process request for orbit around point
        InputActionPtr handleOrbitAroundPoint(IElement* owner, uint8_t button, const base::AbsolutePosition& orbitCenterPos);

        //---

        /// set camera speed factor
        void speedFactor(float speedFactor);

        //--

        // compute camera settings
        void computeRenderingCamera(rendering::scene::CameraSetup& outCamera) const;

    private:
        base::Vector3 m_internalLinearVelocity;
        base::Angles m_internalAngularVelocity;

        base::Vector3 m_currentPosition;
        base::Angles m_currentRotation;

        float m_speedFactor;
        uint32_t m_stamp;

        //--

        void invalidateCameraStamp();

        //--
    };

} // ui