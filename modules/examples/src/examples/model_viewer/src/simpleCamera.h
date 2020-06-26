/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: viewer #]
***/

#pragma once

namespace viewer
{

    //--

    // camera matrices
    struct SimpleCameraState
    {
        Vector3 WorldPosition;
        Vector3 WorldDirection;

        Matrix WorldToCamera;
        Matrix CameraToScreen;
        Matrix ViewToScreen;
        Matrix WorldToScreen;

        Matrix ScreenToWorld;
        Matrix ScreenToView;
    };

    //--

    /// a simple fly around camera 
    class SimpleCamera
    {
    public:
        SimpleCamera();

        //--

        INLINE const Vector3& position() const { return m_position; }
        INLINE const Angles& rotation() const { return m_rotation; }

        //--

        // place camera at given position/rotation, does not interpolate
        void placeCamera(const Vector3& position, const Angles& rotation);

        // move camera to given position rotation, does interpolate
        void moveCamera(const Vector3& position, const Angles& rotation);

        // process input event, returns true if event was used by the camera
        bool processInput(const base::input::BaseEvent& evt);

        // update camera and simulate flying movement
        void update(float dt);

        // calculate camera rendering state
        void calculateRenderData(uint32_t width, uint32_t height, SimpleCameraState& outCamera) const;

    private:
        enum MovementKey
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

        bool m_keys[KEY_MAX]; // F/B/L/R/U/D/F/S

        Vector3 m_position;
        Angles m_rotation;

        float m_fov;
    };


} // viewer

