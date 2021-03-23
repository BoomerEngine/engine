/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\absolute #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

//--

INLINE ExactPosition::ExactPosition()
{}

INLINE ExactPosition::ExactPosition(double x_, double y_, double z_)
    : x(x_), y(y_), z(z_)
{
}

INLINE ExactPosition::ExactPosition(const Vector3& pos)
    : x(pos.x), y(pos.y), z(pos.z)
{}

INLINE ExactPosition& ExactPosition::operator=(const Vector3& other)
{
    x = other.x;
    y = other.y;
    z = other.z;
    return *this;
}

INLINE ExactPosition ExactPosition::operator+(const Vector3& offset) const
{
    return ExactPosition(x + offset.x, y + offset.y, z + offset.z);
}

INLINE ExactPosition& ExactPosition::operator+=(const Vector3& offset)
{
    x += offset.x;
    y += offset.y;
    z += offset.z;
    return *this;
}

INLINE ExactPosition ExactPosition::operator-(const Vector3& offset) const
{
    return ExactPosition(x - offset.x, y - offset.y, z - offset.z);
}

INLINE ExactPosition& ExactPosition::operator-=(const Vector3& offset)
{
    x -= offset.x;
    y -= offset.y;
    z -= offset.z;
    return *this;
}

INLINE ExactPosition ExactPosition::operator+(const ExactPosition& offset) const
{
    return ExactPosition(x + offset.x, y + offset.y, z + offset.z);
}

INLINE ExactPosition& ExactPosition::operator+=(const ExactPosition& offset)
{
    x += offset.x;
    y += offset.y;
    z += offset.z;
    return *this;
}

INLINE ExactPosition ExactPosition::operator-(const ExactPosition& offset) const
{
    return ExactPosition(x - offset.x, y - offset.y, z - offset.z);
}

INLINE ExactPosition& ExactPosition::operator-=(const ExactPosition& offset)
{
    x -= offset.x;
    y -= offset.y;
    z -= offset.z;
    return *this;
}

INLINE ExactPosition ExactPosition::operator*(float scale) const
{
    return ExactPosition(x * scale, y * scale, z * scale);
}

INLINE ExactPosition& ExactPosition::operator*=(float scale)
{
    x *= scale;
    y *= scale;
    z *= scale;
    return *this;
}

INLINE ExactPosition ExactPosition::operator*(const Vector3& scale) const
{
    return ExactPosition(x * scale.x, y * scale.x, z * scale.x);
}

INLINE ExactPosition& ExactPosition::operator*=(const Vector3& scale)
{
    x *= scale.x;
    y *= scale.x;
    z *= scale.x;
    return *this;
}

INLINE ExactPosition ExactPosition::operator-() const
{
    return ExactPosition(-x, -y, -z);
}

INLINE bool ExactPosition::operator==(const ExactPosition& other) const
{
    return (x == other.x) && (y == other.y) && (z == other.z);
}

INLINE bool ExactPosition::operator!=(const ExactPosition& other) const
{
    return !operator==(other);
}

INLINE bool ExactPosition::operator==(const Vector3& other) const
{
    return ((float)x == other.x) && ((float)y == other.y) && ((float)z == other.z);
}

INLINE bool ExactPosition::operator!=(const Vector3& other) const
{
    return !operator==(other);
}

INLINE ExactPosition::operator Vector3() const
{
    return Vector3((float)x, (float)y, (float)z);
}

INLINE float ExactPosition::distance(const ExactPosition& base) const
{
    return std::sqrtf(squareDistance(base));
}

INLINE float ExactPosition::distance(const Vector3& base) const
{
    return std::sqrtf(squareDistance(base));
}

INLINE double ExactPosition::exactDistance(const ExactPosition& base) const
{
    return std::sqrt(exactSquareDistance(base));
}

INLINE float ExactPosition::squareDistance(const ExactPosition& base) const
{
    float dx = x - base.x;
    float dy = y - base.y;
    float dz = z - base.z;
    return (dx * dx) + (dy * dy) + (dz * dz);
}

INLINE float ExactPosition::squareDistance(const Vector3& base) const
{
    float dx = (float)x - base.x;
    float dy = (float)y - base.y;
    float dz = (float)z - base.z;
    return (dx * dx) + (dy * dy) + (dz * dz);
}

INLINE double ExactPosition::exactSquareDistance(const ExactPosition& base) const
{
    double dx = x - base.x;
    double dy = y - base.y;
    double dz = z - base.z;
    return (dx * dx) + (dy * dy) + (dz * dz);
}

//--

INLINE void ExactPosition::snap(double grid)
{
    x = Snap(x, grid);
    y = Snap(y, grid);
    z = Snap(z, grid);
}

INLINE ExactPosition ExactPosition::snapped(double grid) const
{
    return ExactPosition(
        Snap(x, grid),
        Snap(y, grid),
        Snap(z, grid));
}

//--

END_BOOMER_NAMESPACE()
