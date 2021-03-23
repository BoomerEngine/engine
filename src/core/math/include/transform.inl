/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\transform #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

INLINE Transform::Transform()
    : T(ExactPosition::ZERO())
    , R(Quat::IDENTITY())
    , S(Vector3::ONE())
{}

INLINE Transform::Transform(const ExactPosition& pos)
    : T(pos)
    , R(Quat::IDENTITY())
    , S(Vector3::ONE())
{}

INLINE Transform::Transform(const ExactPosition& pos, const Quat& rot)
    : T(pos)
    , R(rot)
    , S(Vector3::ONE())
{}

INLINE Transform::Transform(const ExactPosition& pos, const Quat& rot, const Vector3& scale)
    : T(pos)
    , R(rot)
    , S(scale)
{}

INLINE void Transform::reset()
{
    T = ExactPosition::ZERO();
    R = Quat::IDENTITY();
    S = Vector3::ONE();
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

END_BOOMER_NAMESPACE()
