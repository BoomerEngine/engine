/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#include "build.h"
#include "viewportCameraSetup.h"
#include "uiScenePanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///--

ViewportCameraSetup::ViewportCameraSetup(const Camera& camera, uint32_t width, uint32_t height)
    : m_camera(camera)
    , m_width(width)
    , m_height(height)
{
    m_invWidth = m_width ? 1.0f / m_width : 0.0f;
    m_invHeight = m_height ? 1.0f / m_height : 0.0f;
}

ViewportCameraSetup::ViewportCameraSetup(const RenderingScenePanel* panel)
    : m_camera(panel->cachedCamera())
    , m_width(panel->cachedDrawArea().size().x)
    , m_height(panel->cachedDrawArea().size().y)
{
    m_invWidth = m_width ? 1.0f / m_width : 0.0f;
    m_invHeight = m_height ? 1.0f / m_height : 0.0f;
}
     
Point ViewportCameraSetup::clientPositionFromNormalizedPosition(const Vector3& normalizedPosition) const
{
    auto cx = normalizedPosition.x * m_width;
    auto cy = normalizedPosition.y * m_height;
    return Point(cx, cy);
}

Vector3 ViewportCameraSetup::normalizedScreenPosition(int x, int y, float z) const
{
    auto wx = x * m_invWidth;
    auto wy = y * m_invHeight;
    return Vector3(wx, wy, z);
}

bool ViewportCameraSetup::worldSpaceRayForClientPixelExact(int x, int y, AbsolutePosition& outStart, Vector3& outDir) const
{
    auto coords = normalizedScreenPosition(x, y, 0.5f);
    Vector3 start;
    if (!m_camera.calcWorldSpaceRay(coords, start, outDir))
        return false;

    outStart = AbsolutePosition(start, Vector3());
    return true;
}

bool ViewportCameraSetup::worldSpaceRayForClientPixel(int x, int y, Vector3& outStart, Vector3& outDir) const
{
    auto coords = normalizedScreenPosition(x, y, 0.5f);
    return m_camera.calcWorldSpaceRay(coords, outStart, outDir);
}

bool ViewportCameraSetup::screenToWorld(const Vector3* normalizedScreenPos, AbsolutePosition* outWorldPosition, uint32_t count) const
{
    for (uint32_t i = 0; i < count; ++i)
    {
        Vector3 worldPos;
        if (!m_camera.projectWorldToScreen(normalizedScreenPos[i], worldPos))
            return false;

        outWorldPosition[i] = AbsolutePosition(worldPos, Vector3::ZERO());
    }

    return true;
}

bool ViewportCameraSetup::worldToScreen(const AbsolutePosition* worldPosition, Vector3* outScreenPosition, uint32_t count) const
{
    for (uint32_t i = 0; i < count; ++i)
    {
        Vector3 screenPos;
        auto simpleWorldPos = worldPosition[i].approximate();
        if (!m_camera.projectWorldToScreen(simpleWorldPos, screenPos))
            return false;

        outScreenPosition[i] = screenPos;
    }

    return true;
}

bool ViewportCameraSetup::worldToClient(const AbsolutePosition* worldPosition, Vector3* outClientPosition, uint32_t count) const
{
    for (uint32_t i = 0; i < count; ++i)
    {
        Vector3 screenPos;
        auto simpleWorldPos = worldPosition[i].approximate();
        if (!m_camera.projectWorldToScreen(simpleWorldPos, screenPos))
            return false;

        outClientPosition[i].x = screenPos.x * m_width;
        outClientPosition[i].y = screenPos.y * m_height;
        outClientPosition[i].z = screenPos.z; // no conversion
    }

    return true;
}

float ViewportCameraSetup::calculateViewportScaleFactor(const AbsolutePosition& worldPosition, bool useDPI) const
{
    auto approximateWorldPos = worldPosition.approximate();
    return m_camera.calcScreenSpaceScalingFactor(approximateWorldPos, m_width, m_height) * 1.0f;// cachedStyleParams().m_scale;
}

///--

END_BOOMER_NAMESPACE_EX(ui)
