/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\plane #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//---

INLINE Plane::Plane()
    : d(0)
{}

INLINE Plane::Plane(float nx, float ny, float nz, float dist)
    : n(nx, ny, nz)
    , d(dist)
{}

INLINE Plane::Plane(const Vector3 &normal, float dist)
    : n(normal)
    , d(dist)
{}

INLINE Plane::Plane(const Vector3 &normal, const Vector3 &point)
    : n(normal)
    , d(-Dot(normal, point))
{}

INLINE Plane::Plane(const Vector3 &a, const Vector3 &b, const Vector3 &c)
    : d(0)
{
    if (SafeTriangleNormal(a, b, c, n))
        d = -Dot(n, a);
}

INLINE float Plane::distance(const Vector3 &point) const
{
    return (point | n) + d;
}

INLINE Vector3 Plane::project(const Vector3& point) const
{
    return point - (n * distance(point));
}

INLINE void Plane::normalize()
{
    auto len  = n.normalize();
    if (len > 0.0f)
        d /= len;
}

INLINE void Plane::flip()
{
    n = -n;
    d = -d;
}

INLINE Plane Plane::fliped() const
{
    return Plane(-n, -d);
}

INLINE Plane Plane::operator-() const
{
    return Plane(-n, -d);
}

INLINE bool Plane::operator==(const Plane& other) const
{
    return (n == other.n) && (d == other.d);
}

INLINE bool Plane::operator!=(const Plane& other) const
{
    return !operator==(other);
}

//-----------------------------------------------------------------------------

INLINE const Plane& Plane::EX()
{
    static Plane ret(1, 0, 0, 0);
    return ret;
}

INLINE const Plane& Plane::EY()
{
    static Plane ret(0, 1, 0, 0);
    return ret;
}

INLINE const Plane& Plane::EZ()
{
    static Plane ret(0, 0, 1, 0);
    return ret;
}

//-----------------------------------------------------------------------------

END_BOOMER_NAMESPACE(base)