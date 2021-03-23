/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#pragma once

#include "gizmo.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

/// renderable/collidable line list for gizmo
class EDITOR_VIEWPORT_API GizmoLineShape : public NoCopy
{
public:
    GizmoLineShape();

    //--

    Array<Vector3> m_lines;

    //--

    // transform points
    void transfom(const GizmoReferenceSpace& space, float scaleFactor);

    /// check collision with list
    bool hitTest(const Point& point, const ui::ViewportCameraSetup& viewport, float hitDistance, float& outMinDistance);

    /// render the lines
    void render(rendering::FrameParams& frame, Color lineColor, float lineWidth);

private:
    bool m_cachedPointsValid = false;
    Array<ExactPosition> m_cachedPoints;
    Array<Vector3> m_cachedRenderPoints;

    bool cacheTransformedPoints(const ui::ViewportCameraSetup& viewport);
    bool cacheRenderPoints(const ui::ViewportCameraSetup& viewport, GizmoRenderMode mode);
};

//--

END_BOOMER_NAMESPACE_EX(ed)
