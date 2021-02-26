/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\point #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

INLINE Point::Point()
    : x(0)
    , y(0)
{}

INLINE Point::Point(int ax, int ay)
    : x(ax)
    , y(ay)
{}

INLINE Point::Point(uint32_t ax, uint32_t ay)
        : x(ax), y(ay)
{}

INLINE Point::Point(const float  ax, float ay)
    : x( (int)round(ax) )
    , y( (int)round(ay) )
{}

INLINE Point::Point(const Vector2& other)
    : x((int) round(other.x) )
    , y((int) round(other.y) )
{}

INLINE Point::Point(const Vector3& other)
    : x((int) round(other.x) )
    , y((int) round(other.y) )
{}

INLINE Point::Point(const Vector4& other)
    : x((int) round(other.x) )
    , y((int) round(other.y) )
{}

INLINE const int& Point::operator()(int i) const
{
    return (&x)[i];
}

INLINE int& Point::operator()(int i)
{
    return (&x)[i];
}

INLINE bool Point::operator==(const Point &other) const
{
    return (x == other.x) && (y == other.y);
}

INLINE bool Point::operator!=(const Point &other) const
{
    return (x != other.x) || (y != other.y);
}

INLINE Point Point::operator+=(const Point &other)
{
    x += other.x;
    y += other.y;
    return *this;
}

INLINE Point Point::operator-=(const Point &other)
{
    x -= other.x;
    y -= other.y;
    return *this;
}

INLINE Point Point::operator+(const Point &other) const
{
    return Point(*this) += other;
}

INLINE Point Point::operator-(const Point &other) const
{
    return Point(*this) -= other;
}

//-----------------------------------------------------------------------------

INLINE float Point::distanceTo(const Point& other) const
{
    return sqrt(squaredDistanceTo(other));
}

INLINE float Point::squaredDistanceTo(const Point& other) const
{
    auto dx = (float)x - (float)other.x;
    auto dy = (float)y - (float)other.y;
    return dx*dx + dy*dy;
}

//-----------------------------------------------------------------------------

END_BOOMER_NAMESPACE()
