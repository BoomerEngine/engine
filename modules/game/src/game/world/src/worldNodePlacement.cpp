/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
*
***/

#include "build.h"
#include "worldNodePlacement.h"

namespace game
{

    //---

    RTTI_BEGIN_TYPE_STRUCT(NodeTemplatePlacement);
        RTTI_BIND_NATIVE_COMPARE(NodeTemplatePlacement);
        RTTI_TYPE_TRAIT().noDestructor().fastCopyCompare();
        RTTI_PROPERTY(T).editable();
        RTTI_PROPERTY(R).editable();
        RTTI_PROPERTY(S).editable();
    RTTI_END_TYPE();

    NodeTemplatePlacement::NodeTemplatePlacement()
        : S(1.0f, 1.0f, 1.0f)
    {}

    NodeTemplatePlacement::NodeTemplatePlacement(const base::Vector3& pos, const base::Angles& rot /*= base::Angles(0,0,0)*/, const base::Vector3& scale /*= base::Vector3(1,1,1)*/)
        : T(pos), R(rot), S(scale)
    {}

    NodeTemplatePlacement::NodeTemplatePlacement(float tx, float ty, float tz, float pitch /*= 0.0f*/, float yaw /*= 0.0f*/, float roll /*= 0.0f*/, float sx /*= 1.0f*/, float sy /*= 1.0f*/, float sz /*= 1.0f*/)
        : T(tx, ty, tz)
        , R(pitch, yaw, roll)
        , S(sx, sy, sz)
    {
    }

    NodeTemplatePlacement::NodeTemplatePlacement(const base::AbsoluteTransform& absoluteTransform)
    {
        T = absoluteTransform.position().approximate();
        R = absoluteTransform.rotation().toRotator();
        S = absoluteTransform.scale();
    }

    base::AbsoluteTransform NodeTemplatePlacement::toAbsoluteTransform() const
    {
        return base::AbsoluteTransform(T, R.toQuat(), S);
    }

    base::Transform NodeTemplatePlacement::toTransform() const
    {
        return base::Transform(T, R.toQuat(), S);
    }

    base::Transform NodeTemplatePlacement::toRelativeTransform(const base::AbsoluteTransform& relativeTo) const
    {
        return toAbsoluteTransform() / relativeTo;
    }

    bool NodeTemplatePlacement::operator==(const NodeTemplatePlacement& other) const
    {
        return (T == other.T) && (R == other.R) && (S == other.S);
    }

    bool NodeTemplatePlacement::operator!=(const NodeTemplatePlacement& other) const
    {
        return !operator==(other);
    }

    void NodeTemplatePlacement::print(base::IFormatStream& f) const
    {
        f.appendf("T[{}, {}, {}] ", T.x, T.y, T.z);
        f.appendf("R[{}, {}, {}] ", R.pitch, R.yaw, R.roll);
        f.appendf("S[{}, {}, {}] ", S.x, S.y, S.z);
    }

    NodeTemplatePlacement theIdentityTransform;

    const NodeTemplatePlacement& NodeTemplatePlacement::IDENTITY()
    {
        return theIdentityTransform;
    }

    //---

} // game
