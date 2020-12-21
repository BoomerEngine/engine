/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#include "build.h"
#include "cameraViewportSetup.h"
#include "renderingScenePanel.h"

namespace ui
{
    ///--

    CameraViewportSetup::CameraViewportSetup(const rendering::scene::Camera& camera, uint32_t width, uint32_t height)
        : m_camera(camera)
        , m_width(width)
        , m_height(height)
    {
        m_invWidth = m_width ? 1.0f / m_width : 0.0f;
        m_invHeight = m_height ? 1.0f / m_height : 0.0f;
    }

    CameraViewportSetup::CameraViewportSetup(const RenderingScenePanel* panel)
        : m_camera(panel->cachedCamera())
        , m_width(panel->cachedDrawArea().size().x)
        , m_height(panel->cachedDrawArea().size().y)
    {
        m_invWidth = m_width ? 1.0f / m_width : 0.0f;
        m_invHeight = m_height ? 1.0f / m_height : 0.0f;
    }
     
    base::Point CameraViewportSetup::clientPositionFromNormalizedPosition(const base::Vector3& normalizedPosition) const
    {
        auto cx = normalizedPosition.x * m_width;
        auto cy = normalizedPosition.y * m_height;
        return base::Point(cx, cy);
    }

    base::Vector3 CameraViewportSetup::normalizedScreenPosition(int x, int y, float z) const
    {
        auto wx = x * m_invWidth;
        auto wy = y * m_invHeight;
        return base::Vector3(wx, wy, z);
    }

    bool CameraViewportSetup::worldSpaceRayForClientPixelExact(int x, int y, base::AbsolutePosition& outStart, base::Vector3& outDir) const
    {
        auto coords = normalizedScreenPosition(x, y, 0.5f);
        base::Vector3 start;
        if (!m_camera.calcWorldSpaceRay(coords, start, outDir))
            return false;

        outStart = base::AbsolutePosition(start, base::Vector3());
        return true;
    }

    bool CameraViewportSetup::worldSpaceRayForClientPixel(int x, int y, base::Vector3& outStart, base::Vector3& outDir) const
    {
        auto coords = normalizedScreenPosition(x, y, 0.5f);
        return m_camera.calcWorldSpaceRay(coords, outStart, outDir);
    }

    bool CameraViewportSetup::screenToWorld(const base::Vector3* normalizedScreenPos, base::AbsolutePosition* outWorldPosition, uint32_t count) const
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            base::Vector3 worldPos;
            if (!m_camera.projectWorldToScreen(normalizedScreenPos[i], worldPos))
                return false;

            outWorldPosition[i] = base::AbsolutePosition(worldPos, base::Vector3::ZERO());
        }

        return true;
    }

    bool CameraViewportSetup::worldToScreen(const base::AbsolutePosition* worldPosition, base::Vector3* outScreenPosition, uint32_t count) const
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            base::Vector3 screenPos;
            auto simpleWorldPos = worldPosition[i].approximate();
            if (!m_camera.projectWorldToScreen(simpleWorldPos, screenPos))
                return false;

            outScreenPosition[i] = screenPos;
        }

        return true;
    }

    bool CameraViewportSetup::worldToClient(const base::AbsolutePosition* worldPosition, base::Vector3* outClientPosition, uint32_t count) const
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            base::Vector3 screenPos;
            auto simpleWorldPos = worldPosition[i].approximate();
            if (!m_camera.projectWorldToScreen(simpleWorldPos, screenPos))
                return false;

            outClientPosition[i].x = screenPos.x * m_width;
            outClientPosition[i].y = screenPos.y * m_height;
            outClientPosition[i].z = screenPos.z; // no conversion
        }

        return true;
    }

    float CameraViewportSetup::calculateViewportScaleFactor(const base::AbsolutePosition& worldPosition, bool useDPI) const
    {
        auto approximateWorldPos = worldPosition.approximate();
        return m_camera.calcScreenSpaceScalingFactor(approximateWorldPos, m_width, m_height) * 1.0f;// cachedStyleParams().m_scale;
    }

    ///--

} // ui