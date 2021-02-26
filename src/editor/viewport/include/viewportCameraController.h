/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

enum class CameraMode : uint8_t
{
    FreePerspective,
    OrbitPerspective,
    FreeOrtho,

    Front, // +X
    Back, // -X
    Left, // +Y
    Right, // -Y
    Top, // +Z
    Bottom, // -Z
};

struct EDITOR_VIEWPORT_API ViewportCameraControllerSettings
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ViewportCameraControllerSettings);

public:
    CameraMode mode = CameraMode::FreePerspective;

    float speedLog = 0.0f;

    float perspectiveFov = 90.0f; // fov for perspective camera
    float orthoZoom = 10.0f; // meters for full screen (viewport size/DPI independent)

    AbsolutePosition origin;
    AbsolutePosition position;
    Angles rotation;

    ViewportCameraControllerSettings();

    bool perspective() const;
    bool ortho() const;
    bool fixed() const;

    float calcRelativeScreenSize(ui::Size viewportSize) const;

    void computeRenderingCamera(ui::Size viewportSize, CameraSetup& outCamera) const;
};

//--

/// simple perspective rendering camera controller
class EDITOR_VIEWPORT_API ViewportCameraController
{
public:
    ViewportCameraController();

    ///--

    // current settings
    INLINE const ViewportCameraControllerSettings& settings() const { return m_settings; }

    // configure camera
    void configure(const ViewportCameraControllerSettings& settings);

    ///---

    /// process mouse wheel
    void processMouseWheel(const input::MouseMovementEvent& evt, float delta);

    /// process mouse event
    InputActionPtr handleGeneralFly(IElement* owner, uint8_t button, float sensitivity = 1.0f);

    /// process request for orbit around point
    InputActionPtr handleOrbitAroundPoint(IElement* owner, uint8_t button, bool updatePosition, float sensitivity = 1.0f);

    /// shift origin
    InputActionPtr handleOriginShift(IElement* owner, uint8_t button, const Vector3& xDir, const Vector3& yDir, float sensitivity = 1.0f);

    //--

private:
    ViewportCameraControllerSettings m_settings;
};

END_BOOMER_NAMESPACE_EX(ui)
