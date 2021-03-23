/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#include "build.h"
#include "gizmoReferenceSpace.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

GizmoReferenceSpace::GizmoReferenceSpace()
    : m_space(GizmoSpace::World)
{
    m_axes[0] = Vector3::EX();
    m_axes[1] = Vector3::EY();
    m_axes[2] = Vector3::EZ();
}

GizmoReferenceSpace::GizmoReferenceSpace(GizmoSpace space, const ExactPosition& rootPoint)
    : m_transform(rootPoint)
    , m_space(space)
{
    m_axes[0] = Vector3::EX();
    m_axes[1] = Vector3::EY();
    m_axes[2] = Vector3::EZ();
}

GizmoReferenceSpace::GizmoReferenceSpace(GizmoSpace space, const Transform& transform)
    : m_transform(transform)
    , m_space(space)
{
    cacheAxes();
}

void GizmoReferenceSpace::cacheAxes()
{
    auto localMatrix = m_transform.toMatrix();
    m_axes[0] = localMatrix.column(0).xyz();
    m_axes[1] = localMatrix.column(1).xyz();
    m_axes[2] = localMatrix.column(2).xyz();
}

ExactPosition GizmoReferenceSpace::calcAbsolutePositionForLocal(const Vector3& localPosition) const
{
    return Transformation(m_transform).transformPoint(localPosition);
}

//--

Transform GizmoReferenceSpace::transform(const Transform& original, const Transform& transform) const
{
    // apply position
    //auto posInFrame = pos.relativeTo(referenceSpace.absoluteTransform().position());
    auto posInFrame = Transformation(absoluteTransform().inverted()).transformPoint(original.T);
    posInFrame = Transformation(transform.R).transformVector(posInFrame);
    posInFrame += transform.T;
    auto newPos = Transformation(absoluteTransform()).transformPoint(posInFrame);

    // apply rotation
    //auto deltaRot = referenceSpace.absoluteTransform().rotation().inverted() * transform.rotation() * referenceSpace.absoluteTransform().rotation();
    auto deltaRot = absoluteTransform().R * transform.R * absoluteTransform().R.inverted();
    auto newRot = deltaRot * original.R;

    // compute scape
    auto newScale = transform.S * original.S;

    // build final transform
    return Transform(newPos, newRot, newScale);
}

//--

END_BOOMER_NAMESPACE_EX(ed)