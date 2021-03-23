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

/// helper class to describe a camera with viewport size
class EDITOR_VIEWPORT_API ViewportCameraSetup : public NoCopy
{
public:
    ViewportCameraSetup(const Camera& camera, uint32_t width, uint32_t height);
    ViewportCameraSetup(const RenderingScenePanel* panel);

    //--

    /// calculate exact camera ray for given client pixel position
    bool worldSpaceRayForClientPixelExact(int x, int y, ExactPosition& outStart, Vector3& outDir) const;

    /// calculate camera ray for given client pixel position
    bool worldSpaceRayForClientPixel(int x, int y, Vector3& outStart, Vector3& outDir) const;

    /// calculate normalized screen position
    Vector3 normalizedScreenPosition(int x, int y, float z = 0.0f) const;

    /// calculate client position for given normalized coordinates
    Point clientPositionFromNormalizedPosition(const Vector3& normalizedPosition) const;

    /// query the position for given normalized screen XY (XY - [0,1], Z-linear depth)
    bool screenToWorld(const Vector3* normalizedScreenPos, ExactPosition* outWorldPosition, uint32_t count) const;

    /// query the screen space position (XY - [0,1], Z-linear depth) for given world position
    bool worldToScreen(const ExactPosition* worldPosition, Vector3* outScreenPosition, uint32_t count) const;

    /// query the client space position (XY - [0,w]x[0xh], Z-linear depth) for given world position
    /// NOTE: this is resolution dependent
    bool worldToClient(const ExactPosition* worldPosition, Vector3* outClientPosition, uint32_t count) const;

    /// calculate viewport scale factor for given world position, keeps stuff constant in size on the screen
    /// NOTE: by default this takes the DPI of the viewport into account for free
    float calculateViewportScaleFactor(const ExactPosition& worldPosition, bool useDPI = true) const;

    //--

private:
    const Camera& m_camera;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    float m_invWidth = 0.0f;
    float m_invHeight = 0.0f;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
