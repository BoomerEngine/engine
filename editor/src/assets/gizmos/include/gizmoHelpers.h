/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#pragma once

#include "gizmo.h"

namespace ed
{
    //--

    /// renderable/collidable line list for gizmo
    class ASSETS_GIZMOS_API GizmoLineShape : public base::NoCopy
    {
    public:
        GizmoLineShape();

        //--

        base::Array<base::Vector3> m_lines;

        //--

        // transform points
        void transfom(const GizmoReferenceSpace& space, float scaleFactor);

        /// check collision with list
        bool hitTest(const base::Point& point, const ui::CameraViewportSetup& viewport, float hitDistance, float& outMinDistance);

        /// render the lines
        void render(rendering::scene::FrameParams& frame, base::Color lineColor, float lineWidth);

    private:
        bool m_cachedPointsValid = false;
        Array<AbsolutePosition> m_cachedPoints;
        Array<base::Vector3> m_cachedRenderPoints;

        bool cacheTransformedPoints(const ui::CameraViewportSetup& viewport);
        bool cacheRenderPoints(const ui::CameraViewportSetup& viewport, GizmoRenderMode mode);
    };

    //--

} // ed
