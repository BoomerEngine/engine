/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: camera #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(rendering::test)

//--

/// a simple fly around camera 
class SimpleCamera
{
public:
    SimpleCamera();

    //--

    INLINE const base::Vector3& position() const { return m_position; }
    INLINE const base::Angles& rotation() const { return m_rotation; }

    //--

    // place camera at given position/rotation, does not interpolate
    void placeCamera(const base::Vector3& position, const base::Angles& rotation);

    // move camera to given position rotation, does interpolate
    void moveCamera(const base::Vector3& position, const base::Angles& rotation);

    // process input event, returns true if event was used by the camera
    bool processInput(const base::input::BaseEvent& evt);

    // update camera and simulate flying movement
    void update(float dt);

    // calculate camera rendering state
    void calculateRenderData(scene::CameraSetup& outCamera) const;

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

    base::Vector3 m_position;
    base::Angles m_rotation;

    float m_fov;
};

END_BOOMER_NAMESPACE(rendering::test)
