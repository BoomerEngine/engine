/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#include "build.h"
#include "gizmoReferenceSpace.h"

namespace ed
{

    //--

    GizmoReferenceSpace::GizmoReferenceSpace()
        : m_space(GizmoSpace::World)
    {
        m_axes[0] = Vector3::EX();
        m_axes[1] = Vector3::EY();
        m_axes[2] = Vector3::EZ();
    }

    GizmoReferenceSpace::GizmoReferenceSpace(GizmoSpace space, const AbsolutePosition& rootPoint)
        : m_transform(rootPoint)
        , m_space(space)
    {
        m_axes[0] = Vector3::EX();
        m_axes[1] = Vector3::EY();
        m_axes[2] = Vector3::EZ();
    }

    GizmoReferenceSpace::GizmoReferenceSpace(GizmoSpace space, const AbsoluteTransform& transform)
        : m_transform(transform)
        , m_space(space)
    {
        cacheAxes();
    }

    void GizmoReferenceSpace::cacheAxes()
    {
        auto localMatrix = m_transform.approximate();
        m_axes[0] = localMatrix.column(0).xyz();
        m_axes[1] = localMatrix.column(1).xyz();
        m_axes[2] = localMatrix.column(2).xyz();
    }

    AbsolutePosition GizmoReferenceSpace::calcAbsolutePositionForLocal(const Vector3& localPosition) const
    {
        return m_transform.transformPointFromSpace(localPosition);
    }

    //--

    AbsoluteTransform GizmoReferenceSpace::transform(const AbsoluteTransform& original, const Transform& transform) const
    {
        // apply position
        //auto posInFrame = pos.relativeTo(referenceSpace.absoluteTransform().position());
        auto posInFrame = absoluteTransform().transformPointToSpace(original.position());
        posInFrame = transform.R.transformVector(posInFrame);
        posInFrame += transform.T;
        auto newPos = absoluteTransform().transformPointFromSpace(posInFrame);

        // apply rotation
        //auto deltaRot = referenceSpace.absoluteTransform().rotation().inverted() * transform.rotation() * referenceSpace.absoluteTransform().rotation();
        auto deltaRot = absoluteTransform().rotation() * transform.R * absoluteTransform().rotation().inverted();
        auto newRot = deltaRot * original.rotation();

        // compute scape
        auto newScale = transform.S * original.scale();

        // build final transform
        return AbsoluteTransform(newPos, newRot, newScale);
    }

    //--

} // ed

