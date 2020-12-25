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
    //--

    /// simple perspective rendering camera controller
    class RENDERING_UI_VIEWPORT_API CameraController
    {
    public:
        CameraController();

        ///--

        // origin around which we will rotate in orbit mode
        INLINE const base::AbsolutePosition& origin() const { return m_origin; }

        // get current (animated) camera position
        INLINE const base::AbsolutePosition& position() const { return m_currentPosition; }

        // get current (animated) camera rotation
        INLINE const base::Angles& rotation() const { return m_currentRotation; }

        // get current camera stamp, changed every time camera moves
        INLINE uint32_t stamp() const { return m_stamp; }

        ///--

        /// reset input state of the controller
        void resetInput();

        /// update camera animation/handling
        void animate(float timeDelta);

        /// move camera to given place
        void moveTo(const base::AbsolutePosition& position, const base::Angles& rotation);

        /// set the orbit origin
        void origin(const base::AbsolutePosition& originPosition);

        ///---

        /// process key event, moves the camera around
        bool processKeyEvent(const base::input::KeyEvent& evt);

        /// process mouse wheel
        void processMouseWheel(const base::input::MouseMovementEvent& evt, float delta);

        /// process mouse event
        InputActionPtr handleGeneralFly(IElement* owner, uint8_t button, float speed = 1.0f, float sensitivity = 1.0f);

        /// process request for orbit around point
        InputActionPtr handleOrbitAroundPoint(IElement* owner, uint8_t button, float sensitivity = 1.0f);

        //--

        // compute camera settings
        void computeRenderingCamera(rendering::scene::CameraSetup& outCamera) const;

        //--

        // load camera settings from persistent data
        void configLoad(const ConfigBlock& block);

        // save camera settings
        void configSave(const ConfigBlock& block) const;

    private:
        base::Vector3 m_internalLinearVelocity;
        base::Angles m_internalAngularVelocity;

        base::AbsolutePosition m_currentPosition;
        base::Angles m_currentRotation;

        base::AbsolutePosition m_origin;

        uint32_t m_stamp;

        //--

        void invalidateCameraStamp();

        //--
    };

} // ui