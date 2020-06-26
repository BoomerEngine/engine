/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\absolute #]
***/

#pragma once

#include "base/reflection/include/reflectionMacros.h"
#include "vector3.h"
#include "quat.h"

namespace base
{
    //--

    INLINE AbsoluteTransform::AbsoluteTransform()
        : m_scale(1,1,1)
    {}

    INLINE AbsoluteTransform::AbsoluteTransform(const AbsolutePosition& pos, const Quat& rotation, Vector3 scale)
        : m_position(pos)
        , m_rotation(rotation)
        , m_scale(scale)
    {}

    INLINE const AbsolutePosition& AbsoluteTransform::position() const
    {
        return m_position;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::position(const AbsolutePosition& pos)
    {
        m_position = pos;
        return *this;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::position(const Vector3& pos)
    {
        m_position = pos;
        return *this;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::position(double x, double y, double z)
    {
        m_position = AbsolutePosition(x,y,z);
        return *this;
    }

    INLINE const Quat& AbsoluteTransform::rotation() const
    {
        return m_rotation;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::rotation(const Quat& rot)
    {
        m_rotation = rot;
        return *this;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::rotation(const Angles& angles)
    {
        m_rotation = angles.toQuat();
        return *this;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::rotation(float pitch, float yaw, float roll)
    {
        m_rotation = Angles(pitch, yaw, roll).toQuat();
        return *this;
    }

    INLINE const Vector3& AbsoluteTransform::scale() const
    {
        return m_scale;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::scale(float scale)
    {
        m_scale = Vector3(scale, scale, scale);
        return *this;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::scale(const Vector3& scale)
    {
        m_scale = scale;
        return *this;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::scale(float x, float y, float z)
    {
        m_scale = Vector3(x,y,z);
        return *this;
    }

    //--

    INLINE AbsoluteTransform& AbsoluteTransform::operator*=(const Transform& relativeTransform)
    {
        m_position += m_rotation.transformVector(m_scale * relativeTransform.translation());
        m_rotation *= relativeTransform.rotation();
        m_scale *= relativeTransform.scale();
        return *this;
    }

    INLINE AbsoluteTransform AbsoluteTransform::operator*(const Transform& relativeTransform) const
    {
        return AbsoluteTransform(*this) *= relativeTransform;
    }

    INLINE AbsoluteTransform& AbsoluteTransform::operator*=(const Quat& rot)
    {
        m_rotation *= rot;
        return *this;
    }

    INLINE AbsoluteTransform AbsoluteTransform::operator*(const Quat& rot) const
    {
        return AbsoluteTransform(*this) *= rot;
    }

    INLINE AbsoluteTransform AbsoluteTransform::operator+(const Vector3& delta) const
    {
        return AbsoluteTransform(m_position + delta, m_rotation, m_scale);
    }

    INLINE AbsoluteTransform& AbsoluteTransform::operator+=(const Vector3& delta)
    {
        m_position += delta;
        return *this;
    }

    INLINE AbsoluteTransform AbsoluteTransform::operator-(const Vector3& delta) const
    {
        return AbsoluteTransform(m_position - delta, m_rotation, m_scale);
    }

    INLINE AbsoluteTransform& AbsoluteTransform::operator-=(const Vector3& delta)
    {
        m_position -= delta;
        return *this;
    }

    INLINE bool AbsoluteTransform::operator==(const AbsoluteTransform& other) const
    {
        return (m_position == other.m_position) && (m_rotation == other.m_rotation) && (m_scale == other.m_scale);
    }

    INLINE bool AbsoluteTransform::operator!=(const AbsoluteTransform& other) const
    {
        return !operator==(other);
    }

    //--

} // scene
