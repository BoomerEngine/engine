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
        : m_trans(Translation::ZERO())
        , m_rot(Rotation::IDENTITY())
        , m_scale(Scale::ONE())
    {}

    INLINE Transform::Transform(const Translation& pos)
        : m_trans(pos)
        , m_rot(Rotation::IDENTITY())
        , m_scale(Scale::ONE())
    {}

    INLINE Transform::Transform(const Translation& pos, const Rotation& rot)
        : m_trans(pos)
        , m_rot(rot)
        , m_scale(Scale::ONE())
    {}

    INLINE Transform::Transform(const Translation& pos, const Rotation& rot, const Scale& scale)
        : m_trans(pos)
        , m_rot(rot)
        , m_scale(scale)
    {}

    INLINE Transform Transform::T(const Translation& t)
    {
        return Transform(t);
    }

    INLINE Transform Transform::R(const Rotation& r)
    {
        return Transform(Translation::ZERO(), r);
    }

    INLINE Transform Transform::TR(const Translation& t, const Rotation& r)
    {
        return Transform(t, r);
    }

    INLINE Transform Transform::S(const Scale& s)
    {
        return (s.isSimilar(Scale::ONE())) ? Transform() : Transform(Translation::ZERO(), Rotation::IDENTITY(), s);
    }

    INLINE Transform Transform::S(float s)
    {
        return (s == 1.0f) ? Transform() : Transform(Translation::ZERO(), Rotation::IDENTITY(), Scale(s,s,s));
    }

    INLINE Transform Transform::TRS(const Translation& t, const Rotation& r, const Scale& s)
    {
        return Transform(t,r,s);
    }

    INLINE Transform Transform::TRS(const Translation& t, const Rotation& r, float s)
    {
        return Transform(t,r,Scale(s,s,s));
    }

    INLINE Transform& Transform::identity()
    {
        m_trans = Translation::ZERO();
        m_rot = Rotation::IDENTITY();
        m_scale = Scale::ONE();
        return *this;
    }

    INLINE Transform& Transform::shiftParent(const Translation& translationInParentSpace)
    {
        m_trans += translationInParentSpace;
        return *this;
    }

    INLINE Transform& Transform::shiftLocal(const Translation& translationInLocalSpace)
    {
        m_trans += m_rot.transformVector(translationInLocalSpace * m_scale);
        return *this;
    }

    INLINE Transform& Transform::rotateParent(float pitch, float yaw, float roll)
    {
        m_rot = m_rot * Angles(pitch, yaw, roll).toQuat();
        return *this;
    }

    INLINE Transform& Transform::rotateParent(const base::Quat& quat)
    {
        m_rot = m_rot * quat;
        return *this;
    }

    INLINE Transform& Transform::rotateLocal(float pitch, float yaw, float roll)
    {
        m_rot = Angles(pitch, yaw, roll).toQuat() * m_rot;
        return *this;
    }

    INLINE Transform& Transform::rotateLocal(const base::Quat& quat)
    {
        m_rot = quat * m_rot;
        return *this;
    }

    INLINE bool Transform::operator==(const Transform& other) const
    {
        return (m_trans == other.m_trans) && (m_rot == other.m_rot) && (m_scale == other.m_scale);
    }

    INLINE bool Transform::operator!=(const Transform& other) const
    {
        return !operator==(other);
    }

    INLINE Transform& Transform::translation(const Translation& t)
    {
        m_trans = t;
        return *this;
    }

    INLINE Transform& Transform::translation(float x, float y, float z)
    {
        m_trans = Translation(x,y,z);
        return *this;
    }

    INLINE Transform& Transform::rotation(const Rotation& rotation)
    {
        m_rot = rotation;
        return *this;
    }

    INLINE Transform& Transform::rotation(float pitch, float yaw, float roll)
    {
        m_rot = Angles(pitch, yaw, roll).toQuat();
        return *this;
    }

    INLINE Transform& Transform::scale(const Scale& s)
    {
        m_scale = s;
        return *this;
    }

    INLINE Transform& Transform::scale(float s)
    {
        m_scale = Scale(s,s,s);
        return *this;
    }

    //--

} // base