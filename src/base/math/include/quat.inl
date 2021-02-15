/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\quat #]
***/

#pragma once

namespace base
{
    //---

    INLINE Quat::Quat()
        : x(0), y(0), z(0), w(1)
    {}

    INLINE Quat::Quat(float inX, float inY, float inZ, float inW)
        : x(inX), y(inY), z(inZ), w(inW)
    {}

    INLINE Quat::Quat(const Vector4 &other)
        : x(other.x), y(other.y), z(other.z), w(other.w)
    {}

    INLINE void Quat::identity()
    {
        x = y = z = 0.0f;
        w = 1.0f;
    }

    INLINE void Quat::invert()
    {
        w = -w;
    }

    INLINE Quat Quat::inverted() const
    {
        return Quat(x, y, z, -w);
    }

    INLINE void Quat::normalize()
    {
        float len = 1.0f / std::sqrt(x*x + y*y + z*z + w*w);
        x *= len;
        y *= len;
        z *= len;
        w *= len;
    }

    INLINE void Quat::align(const Quat& other)
    {
        if (Dot(*this, other) < 0.0f)
        {
            x = -x;
            y = -y;
            z = -z;
            w = -w;
        }
    }

    INLINE Quat Quat::operator-() const
    {
        return inverted();
    }

    INLINE Quat& Quat::operator*=(const Quat &other)
    {
        *this = Concat(*this, other);
        return *this;
    };

    INLINE Quat Quat::operator*(const Quat &other) const
    {
        return Concat(*this, other);
    }

    INLINE bool Quat::operator==(const Quat &other) const
    {
        return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w);
    }

    INLINE bool Quat::operator!=(const Quat &other) const
    {
        return (x != other.x) || (y != other.y) || (z != other.z) || (w != other.w);
    }

    //---

} // base