/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\transform #]
***/

#pragma once

namespace base
{
    //--

    INLINE Transform::Transform()
        : T(Translation::ZERO())
        , R(Rotation::IDENTITY())
        , S(Scale::ONE())
    {}

    INLINE Transform::Transform(const Translation& pos)
        : T(pos)
        , R(Rotation::IDENTITY())
        , S(Scale::ONE())
    {}

    INLINE Transform::Transform(const Translation& pos, const Rotation& rot)
        : T(pos)
        , R(rot)
        , S(Scale::ONE())
    {}

    INLINE Transform::Transform(const Translation& pos, const Rotation& rot, const Scale& scale)
        : T(pos)
        , R(rot)
        , S(scale)
    {}

    INLINE Transform& Transform::identity()
    {
        T = Translation::ZERO();
        R = Rotation::IDENTITY();
        S = Scale::ONE();
        return *this;
    }

    INLINE Transform& Transform::shiftParent(const Translation& translationInParentSpace)
    {
        T += translationInParentSpace;
        return *this;
    }

    INLINE Transform& Transform::shiftLocal(const Translation& translationInLocalSpace)
    {
        T += R.transformVector(translationInLocalSpace * S);
        return *this;
    }

    INLINE Transform& Transform::rotateParent(float pitch, float yaw, float roll)
    {
        R = R * Angles(pitch, yaw, roll).toQuat();
        return *this;
    }

    INLINE Transform& Transform::rotateParent(const base::Quat& quat)
    {
        R = R * quat;
        return *this;
    }

    INLINE Transform& Transform::rotateLocal(float pitch, float yaw, float roll)
    {
        R = Angles(pitch, yaw, roll).toQuat() * R;
        return *this;
    }

    INLINE Transform& Transform::rotateLocal(const base::Quat& quat)
    {
        R = quat * R;
        return *this;
    }

    INLINE bool Transform::operator==(const Transform& other) const
    {
        return (T == other.T) && (R == other.R) && (S == other.S);
    }

    INLINE bool Transform::operator!=(const Transform& other) const
    {
        return !operator==(other);
    }

    //--

} // base