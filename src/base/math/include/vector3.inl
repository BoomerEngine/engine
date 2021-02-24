/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\vector3 #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//-----------------------------------------------------------------------------

INLINE Vector3::Vector3()
    : x(0)
    , y(0)
    , z(0)
{}

INLINE Vector3::Vector3(float inX, float inY, float inZ)
    : x(inX)
    , y(inY)
    , z(inZ)
{}

INLINE float Vector3::minValue() const
{
    return std::min(x, std::min(y, z));
}

INLINE float Vector3::maxValue() const
{
    return std::max(x, std::max(y, z));
}

INLINE float Vector3::sum() const
{
    return x + y + z;
}

INLINE float Vector3::trace() const
{
    return x * y * z;
}

INLINE Vector3 Vector3::abs() const
{
    return Vector3(std::abs(x), std::abs(y), std::abs(z));
}

INLINE Vector3 Vector3::round() const
{
    return Vector3(std::round(x), std::round(y), std::round(z));
}

INLINE Vector3 Vector3::frac() const
{
    return Vector3(x - std::trunc(x), y - std::trunc(y), z - std::trunc(z));
}

INLINE Vector3 Vector3::trunc() const
{
    return Vector3(std::trunc(x), std::trunc(y), std::trunc(z));
}

INLINE Vector3 Vector3::floor() const
{
    return Vector3(std::floor(x), std::floor(y), std::floor(z));
}

INLINE Vector3 Vector3::ceil() const
{
    return Vector3(std::ceil(x), std::ceil(y), std::ceil(z));
}

INLINE float Vector3::squareLength() const
{
    return x*x + y*y + z*z;
}

INLINE float Vector3::length() const
{
    return std::hypot(x, y, z);
}

INLINE float Vector3::invLength() const
{
    auto len = length();
    return (len > 0.0f) ? (1.0f / len) : 0.0f;
}

INLINE bool Vector3::isNearZero(const  float epsilon)  const
{
    return (x >= -epsilon && x <= epsilon) && (y >= -epsilon && y <= epsilon) && (z >= -epsilon && z <= epsilon);
}

INLINE bool Vector3::isSimilar(const Vector3 &other, float epsilon) const
{
    float dx = x - other.x;
    float dy = y - other.y;
    float dz = z - other.z;
    return (dx >= -epsilon && dx <= epsilon) && (dy >= -epsilon && dy <= epsilon) && (dz >= -epsilon && dz <= epsilon);
}

INLINE bool Vector3::isZero() const
{
    return !x && !y && !z;
}

INLINE float Vector3::normalize()
{
    auto len = std::hypot(x,y,z);
    if (len > NORMALIZATION_EPSILON)
    {
        x /= len;
        y /= len;
        z /= len;
    }
    return len;
}

INLINE float Vector3::normalizeFast()
{
    auto len = std::hypot(x,y,z);
    auto rcp = 1.0f / len;
    x *= rcp;
    y *= rcp;
    z *= rcp;
    return len;
}

INLINE Vector3 Vector3::normalized() const
{
    Vector3 ret(*this);
    ret.normalize();
    return ret;
}

INLINE Vector3 Vector3::normalizedFast() const
{
    Vector3 ret(*this);
    ret.normalizeFast();
    return ret;
}

INLINE Vector3 Vector3::oneOver() const
{
    return Vector3(x != 0.0f ? 1.0f / x : 0.0f,
                    y != 0.0f ? 1.0f / y : 0.0f,
                    z != 0.0f ? 1.0f / z : 0.0f);
}

INLINE Vector3 Vector3::oneOverFast() const
{
    return Vector3(1.0f / x, 1.0f / y, 1.0f / z);
}

INLINE Vector2 Vector3::xx() const { return Vector2(x,x); }
INLINE Vector2 Vector3::yy() const { return Vector2(y,y); }
INLINE Vector2 Vector3::zz() const { return Vector2(z,z); }
INLINE Vector2 Vector3::yx() const { return Vector2(y,x); }
INLINE Vector2 Vector3::xz() const { return Vector2(x,z); }
INLINE Vector2 Vector3::zx() const { return Vector2(z, x); }
INLINE Vector2 Vector3::zy() const { return Vector2(z, y); }

INLINE Vector3 Vector3::xxx() const { return Vector3(x,x,x); }
INLINE Vector3 Vector3::xxy() const { return Vector3(x,x,y); }
INLINE Vector3 Vector3::xxz() const { return Vector3(x,x,z); }
INLINE Vector3 Vector3::xyx() const { return Vector3(x,y,x); }
INLINE Vector3 Vector3::xyy() const { return Vector3(x,y,y); }
INLINE Vector3 Vector3::xzx() const { return Vector3(x,z,x); }
INLINE Vector3 Vector3::xzy() const { return Vector3(x,z,y); }
INLINE Vector3 Vector3::xzz() const { return Vector3(x,z,z); }
INLINE Vector3 Vector3::yxx() const { return Vector3(y,x,x); }
INLINE Vector3 Vector3::yxy() const { return Vector3(y,x,y); }
INLINE Vector3 Vector3::yxz() const { return Vector3(y,x,z); }
INLINE Vector3 Vector3::yyx() const { return Vector3(y,y,x); }
INLINE Vector3 Vector3::yyy() const { return Vector3(y,y,y); }
INLINE Vector3 Vector3::yyz() const { return Vector3(y,y,z); }
INLINE Vector3 Vector3::yzx() const { return Vector3(y,z,x); }
INLINE Vector3 Vector3::yzy() const { return Vector3(y,z,y); }
INLINE Vector3 Vector3::yzz() const { return Vector3(y,z,z); }
INLINE Vector3 Vector3::zxx() const { return Vector3(z,x,x); }
INLINE Vector3 Vector3::zxy() const { return Vector3(z,x,y); }
INLINE Vector3 Vector3::zxz() const { return Vector3(z,x,z); }
INLINE Vector3 Vector3::zyx() const { return Vector3(z,y,x); }
INLINE Vector3 Vector3::zyy() const { return Vector3(z,y,y); }
INLINE Vector3 Vector3::zyz() const { return Vector3(z,y,z); }
INLINE Vector3 Vector3::zzx() const { return Vector3(z,z,x); }
INLINE Vector3 Vector3::zzy() const { return Vector3(z,z,y); }
INLINE Vector3 Vector3::zzz() const { return Vector3(z,z,z); }

INLINE const Vector2& Vector3::xy() const { return *(const Vector2*) &x; }
INLINE const Vector2& Vector3::yz() const { return *(const Vector2*) &y; }
INLINE const Vector3& Vector3::xyz() const { return *this; }

INLINE Vector2& Vector3::xy() { return *(Vector2*) &x; }
INLINE Vector2& Vector3::yz() { return *(Vector2*) &y; }
INLINE Vector3& Vector3::xyz()  { return *this; }

INLINE Vector2 Vector3::_xy() const { return xy(); }
INLINE Vector2 Vector3::_yz() const { return yz(); }
INLINE Vector3 Vector3::_xyz() const { return xyz(); }

INLINE Vector4 Vector3::xyzw(float w) const
{
    return Vector4(x,y,z,w);
}

INLINE Vector3 Vector3::transform(const Vector3& vx, const Vector3& vy, const Vector3& vz) const
{
    return (x * vx) + (y * vy) + (z * vz);
}

INLINE float Vector3::distance(const Vector3 &other) const
{
    return std::hypot(x - other.x, y - other.y, z - other.z);
}

INLINE float Vector3::squareDistance(const Vector3 &other) const
{
    return (x - other.x) * (x - other.x) + (y - other.y)*(y - other.y) + (z - other.z)*(z - other.z);
}

INLINE Vector3 Vector3::operator-() const
{
    return Vector3(-x, -y, -z);
}

INLINE Vector3 Vector3::operator+(const Vector3 &other) const
{
    return Vector3(x + other.x, y + other.y, z + other.z);
}

INLINE Vector3 Vector3::operator+(float value) const
{
    return Vector3(x + value, y + value, z + value);
}

INLINE Vector3 Vector3::operator-(const Vector3 &other) const
{
    return Vector3(x - other.x, y - other.y, z - other.z);
}

INLINE Vector3 Vector3::operator-(float value) const
{
    return Vector3(x - value, y - value, z - value);
}

INLINE Vector3 Vector3::operator*(const Vector3 &other) const
{
    return Vector3(x*other.x, y*other.y, z*other.z);
}

INLINE Vector3 Vector3::operator*(float value) const
{
    return Vector3(x*value, y*value, z*value);
}

INLINE Vector3 operator*(float value, const Vector3 &other)
{
    return Vector3(value*other.x, value*other.y, value*other.z);
}

INLINE Vector3 Vector3::operator/(const Vector3 &other) const
{
    return Vector3(x / other.x, y / other.y, z / other.z);
}

INLINE Vector3 Vector3::operator/(float value) const
{
    return Vector3(x / value, y / value, z / value);
}

INLINE Vector3& Vector3::operator+=(const Vector3 &other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

INLINE Vector3& Vector3::operator+=(float value)
{
    x += value;
    y += value;
    z += value;
    return *this;
}

INLINE Vector3& Vector3::operator-=(const Vector3 &other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

INLINE Vector3& Vector3::operator-=(float value)
{
    x -= value;
    y -= value;
    z -= value;
    return *this;
}

INLINE Vector3& Vector3::operator*=(const Vector3 &other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
}

INLINE Vector3& Vector3::operator*=(float value)
{
    x *= value;
    y *= value;
    z *= value;
    return *this;
}

INLINE Vector3& Vector3::operator/=(const Vector3 &other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

INLINE Vector3& Vector3::operator/=(float value)
{
    x /= value;
    y /= value;
    z /= value;
    return *this;
}

INLINE bool Vector3::operator==(const Vector3 &other) const
{
    return x == other.x && y == other.y && z == other.z;
}

INLINE bool Vector3::operator!=(const Vector3 &other) const
{
    return x != other.x || y != other.y || z != other.z;
}

INLINE float Vector3::operator|(const Vector3 &other) const
{
    return (x*other.x) + (y*other.y) + (z*other.z);
}

INLINE Vector3 Vector3::operator^(const Vector3 &other) const
{
    return Cross(*this, other);
}

INLINE float Vector3::operator[](int index) const
{
    return ((const float*)this)[index];
}

INLINE float& Vector3::operator[](int index)
{
    return ((float*)this)[index];
}

INLINE int Vector3::largestAxis() const
{
    float ax = std::abs(x);
    float ay = std::abs(y);
    float az = std::abs(z);

    if (ax > ay && ax > az) return 0;
    if (ay > ax && ay > az) return 1;
    return 2;
}

INLINE int Vector3::smallestAxis() const
{
    float ax = std::abs(x);
    float ay = std::abs(y);
    float az = std::abs(z);

    if (ax < ay && ax < az) return 0;
    if (ay < ax && ay < az) return 1;
    return 2;
}

//--

END_BOOMER_NAMESPACE(base)