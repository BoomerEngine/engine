/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#include "build.h"
#include "gizmoHelpers.h"
#include "gizmoReferenceSpace.h"

#include "viewportCameraSetup.h"
#include "engine/rendering/include/debug.h"
#include "engine/rendering/include/params.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

static const uint32_t POINT_CACHE = 128;

//--

GizmoLineShape::GizmoLineShape()
{}

void GizmoLineShape::transfom(const GizmoReferenceSpace& space, float scaleFactor)
{
    m_cachedPoints.reset();
    m_cachedRenderPoints.reset();

    const auto pointCount = m_lines.size();
    m_cachedPoints.allocateUninitialized(pointCount);
    m_cachedRenderPoints.allocateUninitialized(pointCount);
    m_cachedPointsValid = true;

    Transformation t(space.absoluteTransform());

    for (uint32_t i = 0; i < pointCount; ++i)
    {
        m_cachedPoints.typedData()[i] = t.transformPoint(m_lines.typedData()[i] * scaleFactor);
        m_cachedRenderPoints.typedData()[i] = m_cachedPoints.typedData()[i];
    }
}

bool GizmoLineShape::hitTest(const Point& point, const ui::ViewportCameraSetup& viewport, float hitDistance, float& outMinDistance)
{
    const auto mousePosVector = point.toVector();

    bool hasHit = false;

    if (m_cachedPointsValid)
    {
        const auto pointCount = m_cachedPoints.size();
        for (uint32_t i = 0; i < pointCount; i += 2)
        {
            Vector3 screenPoints[2];
            if (viewport.worldToClient(m_cachedPoints.typedData() + i, screenPoints, 2))
            {
                const auto dist = IGizmo::CalcDistanceToSegment(mousePosVector, screenPoints[0].xy(), screenPoints[1].xy());
                if (dist < hitDistance)
                {
                    outMinDistance = std::min(outMinDistance, dist);
                    hasHit = true;
                }
            }
        }
    }

    return hasHit;
}

void GizmoLineShape::render(DebugGeometryCollector& debug, Color lineColor, float lineWidth)
{
    if (m_cachedPointsValid)
    {
        /*DebugDrawer dd(frame.geometry.overlay);
        dd.color(lineColor);
        dd.lines(m_cachedRenderPoints.typedData(), m_cachedRenderPoints.size());*/
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
