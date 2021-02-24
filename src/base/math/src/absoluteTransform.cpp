/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\absolute #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

RTTI_BEGIN_TYPE_STRUCT(AbsoluteTransform);
    RTTI_BIND_NATIVE_COMPARE(AbsoluteTransform);
    RTTI_PROPERTY(m_position);
    RTTI_PROPERTY(m_rotation);
    RTTI_PROPERTY(m_scale);
RTTI_END_TYPE();

//--

Matrix AbsoluteTransform::operator/(const AbsolutePosition& basePosition) const
{
    return Matrix::BuildTRS(m_position - basePosition, m_rotation, m_scale);
}

Matrix AbsoluteTransform::approximate() const
{
    return Matrix::BuildTRS(position().approximate(), rotation(), scale());
}

Transform AbsoluteTransform::operator/(const AbsoluteTransform& base) const
{
    auto& obj = *this;

    // Basic transform equations
    //obj.position = base.position + base.rotation.transformVector(base.scale * rel.pos);
    //obj.rotation = rel.rotation * base.rotation;
    //obj.scale = base.scale * rel.scale;

    // To get the relative position between two absolute transforms:
    //obj.position = base.position + base.rotation.transformVector(base.scale * rel.pos);
    //obj.position - base.position = base.rotation.transformVector(base.scale * rel.pos);
    //base.rotation.transformInvVector(obj.position - base.position) = (base.scale * rel.pos);
    //(base.rotation.transformInvVector(obj.position - base.position) / base.scale = rel.pos;
    auto pos = base.rotation().transformInvVector(obj.position() - base.position()) / base.scale();

    // To get the relative rotation between two absolute transforms:
    //obj.rotation = base.rotation * rel.rotation;
    //base.rotation.inverted() * obj.rotation = rel.rotation;
    auto rot = base.rotation().inverted() * obj.rotation();

    // To get the relative scale between two absolute transforms:
    //obj.scale = base.scale * rel.scale;
    //obj.scale * base.scale.inverted() = rel.scale;
    auto scale = base.scale().oneOverFast() * obj.scale();

    // compose final transform
    return Transform(pos, rot, scale);
}

AbsolutePosition AbsoluteTransform::transformPointFromSpace(const base::Vector3 &localPoint) const
{
    return m_position + m_rotation.transformVector(localPoint * m_scale);
}

Vector3 AbsoluteTransform::transformDirectionFromSpace(const Vector3& localDir) const
{
    return m_rotation.transformVector(localDir); // TODO: support scale properly
}

Vector3 AbsoluteTransform::transformPointToSpace(const base::AbsolutePosition &absolutePosition) const
{
    return m_rotation.transformInvVector(absolutePosition - m_position) / m_scale;
}

Vector3 AbsoluteTransform::transformDirectionToSpace(const Vector3& worldDir) const
{
    return m_rotation.transformInvVector(worldDir);
}

Box AbsoluteTransform::transformBox(const Box& localBox) const
{
    // empty box is never transformed
    if (localBox.empty())
        return Box::EMPTY();

    // Start with first corner
    Box out;
    out.min = out.max = transformPointFromSpace(localBox.corner(0)).approximate();
    // Transform ret of the corners
    out.merge(transformPointFromSpace(localBox.corner(1)).approximate());
    out.merge(transformPointFromSpace(localBox.corner(2)).approximate());
    out.merge(transformPointFromSpace(localBox.corner(3)).approximate());
    out.merge(transformPointFromSpace(localBox.corner(4)).approximate());
    out.merge(transformPointFromSpace(localBox.corner(5)).approximate());
    out.merge(transformPointFromSpace(localBox.corner(6)).approximate());
    out.merge(transformPointFromSpace(localBox.corner(7)).approximate());
    return out;
}

//--

AbsoluteTransform Lerp(const AbsoluteTransform& a, const AbsoluteTransform& b, float frac)
{
    auto pos = Lerp(a.position(), b.position(), frac);
    auto rot = Lerp(a.rotation(), b.rotation(), frac);
    auto scale = Lerp(a.scale(), b.scale(), frac);
    return AbsoluteTransform(pos, rot, scale);
}

//--

static const AbsoluteTransform ROOT_ABST;

const AbsoluteTransform& AbsoluteTransform::ROOT()
{
    return ROOT_ABST;
}

//--

END_BOOMER_NAMESPACE(base)