/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\vector4 #]
***/

#pragma once

namespace base
{
    //--

    INLINE Vector4::Vector4()
        : x(0), y(0), z(0), w(0)
    {}

    INLINE Vector4::Vector4(float inX, float inY, float inZ, float inW)
        : x(inX), y(inY), z(inZ), w(inW)
    {}

    INLINE Vector4::Vector4(const Vector3 &other, float inW /*= 1.0f*/)
        : x(other.x), y(other.y), z(other.z), w(inW)
    {}

    INLINE float Vector4::minValue() const
    {
        return std::min(std::min(x, y), std::min(z, w));
    }

    INLINE float Vector4::maxValue() const
    {
        return std::max(std::max(x, y), std::max(z, w));
    }

    INLINE float Vector4::sum() const
    {
        return x + y + z + w;
    }

    INLINE float Vector4::trace() const
    {
        return x * y * z * w;
    }

    INLINE Vector4 Vector4::abs() const
    {
        return Vector4(std::abs(x), std::abs(y), std::abs(z), std::abs(w));
    }

    INLINE Vector4 Vector4::round() const
    {
        return Vector4(std::round(x), std::round(y), std::round(z), std::round(w));
    }

    INLINE Vector4 Vector4::frac() const
    {
        return Vector4(x - std::trunc(x), y - std::trunc(y), z - std::trunc(z), w - std::trunc(w));
    }

    INLINE Vector4 Vector4::trunc() const
    {
        return Vector4(std::trunc(x), std::trunc(y), std::trunc(z), std::trunc(w));
    }

    INLINE Vector4 Vector4::floor() const
    {
        return Vector4(std::floor(x), std::floor(y), std::floor(z), std::floor(w));
    }

    INLINE Vector4 Vector4::ceil() const
    {
        return Vector4(std::ceil(x), std::ceil(y), std::ceil(z), std::ceil(w));
    }

    INLINE float Vector4::squareLength() const
    {
        return x*x + y*y + z*z + w*w;
    }

    INLINE float Vector4::length() const
    {
        return sqrt(x*x + y*y + z*z + w*w);
    }

    INLINE float Vector4::invLength() const
    {
        auto len  = length();
        return (len > 0.0f) ? 1.0f / len : 0.0f;
    }

    INLINE bool Vector4::isNearZero(float epsilon/*=std::numeric_limits<float>::epsilon()*/) const
    {
        return (x >= -epsilon && x <= epsilon) && (y >= -epsilon && y <= epsilon) && (z >= -epsilon && z <= epsilon) && (w >= -epsilon && w <= epsilon);
    }

    INLINE bool Vector4::isSimilar(const Vector4 &other, float epsilon/*=std::numeric_limits<float>::epsilon()*/) const
    {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        float dw = w - other.w;
        return (dx >= -epsilon && dx <= epsilon) && (dy >= -epsilon && dy <= epsilon) && (dz >= -epsilon && dz <= epsilon) && (dw >= -epsilon && dw <= epsilon);
    }

    INLINE bool Vector4::isZero() const
    {
        return !x && !y && !z && !w;
    }

    INLINE float Vector4::normalize()
    {
        float len = sqrt(x*x + y*y + z*z + w*w);
        if (len > NORMALIZATION_EPSILON)
        {
            x /= len;
            y /= len;
            z /= len;
            w /= len;
        }
        return len;
    }

    INLINE Vector4 Vector4::normalized() const
    {
        Vector4 ret(*this);
        ret.normalize();
        return ret;
    }

    INLINE float Vector4::normalizeFast()
    {
        auto len  = length();
        if (len >= NORMALIZATION_EPSILON)
        {
            auto rcp = 1.0f / len;
            x *= rcp;
            y *= rcp;
            z *= rcp;
            w *= rcp;
        }
        return len;
    }

    INLINE Vector4 Vector4::normalizedFast() const
    {
        Vector4 ret(*this);
        ret.normalizeFast();
        return ret;
    }

    INLINE void Vector4::project()
    {
        if (w != 0.0f)
        {
            x /= w;
            y /= w;
            z /= w;
            w = 1.0f;
        }
    }

    INLINE void Vector4::projectFast()
    {
        auto oneOver = 1.0f / w;
        x *= oneOver;
        y *= oneOver;
        z *= oneOver;
        w = 1.0f;
    }

    INLINE Vector3 Vector4::projected() const
    {
        if (w != 0.0f)
            return Vector3(x/w, y/w, z/w);
        return Vector3::ZERO();
    }

    INLINE Vector3 Vector4::projectedFast() const
    {
        auto oneOver = 1.0f / w;
        return Vector3(x * oneOver, y * oneOver, z * oneOver);
    }

    INLINE Vector4 Vector4::oneOver() const
    {
        return Vector4(
                x != 0.0f ? 1.0f / x : 0.0f,
                y != 0.0f ? 1.0f / y : 0.0f,
                z != 0.0f ? 1.0f / z : 0.0f,
                w != 0.0f ? 1.0f / w : 0.0f);
    }

    INLINE Vector4 Vector4::oneOverFast() const
    {
        return Vector4(1.0f / x, 1.0f / y, 1.0f / z, 1.0f / w);
    }

    INLINE float Vector4::distance(const Vector4 &other) const
    {
        return sqrt((x - other.x) * (x - other.x) + (y - other.y)*(y - other.y) + (z - other.z)*(z - other.z) + (w - other.w)*(w - other.w));
    }

    INLINE float Vector4::squareDistance(const Vector4 &other) const
    {
        return Dot(*this, other);
    }

    INLINE Vector4 Vector4::operator-() const
    {
        return Vector4(-x, -y, -z, -w);
    }

    INLINE Vector4 Vector4::operator+(const Vector4 &other) const
    {
        return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    INLINE Vector4 Vector4::operator+(float value) const
    {
        return Vector4(x + value, y + value, z + value, w + value);
    }

    INLINE Vector4 Vector4::operator-(const Vector4 &other) const
    {
        return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    INLINE Vector4 Vector4::operator-(float value) const
    {
        return Vector4(x - value, y - value, z - value, w - value);
    }

    INLINE Vector4 Vector4::operator*(const Vector4 &other) const
    {
        return Vector4(x*other.x, y*other.y, z*other.z, w*other.w);
    }

    INLINE Vector4 Vector4::operator*(float value) const
    {
        return Vector4(x*value, y*value, z*value, w*value);
    }

    INLINE Vector4 operator*(float value, const Vector4 &other)
    {
        return Vector4(value*other.x, value*other.y, value*other.z, value*other.w);
    }

    INLINE Vector4 Vector4::operator/(const Vector4 &other) const
    {
        return Vector4(x / other.x, y / other.y, z / other.z, w / other.w);
    }

    INLINE Vector4 Vector4::operator/(float value) const
    {
        auto oneOver = 1.0f / value;
        return Vector4(x * oneOver, y * oneOver, z * oneOver, w * oneOver);
    }

    INLINE Vector4& Vector4::operator+=(const Vector4 &other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    INLINE Vector4& Vector4::operator+=(float value)
    {
        x += value;
        y += value;
        z += value;
        w += value;
        return *this;
    }

    INLINE Vector4& Vector4::operator-=(const Vector4 &other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    INLINE Vector4& Vector4::operator-=(float value)
    {
        x -= value;
        y -= value;
        z -= value;
        w -= value;
        return *this;
    }

    INLINE Vector4& Vector4::operator*=(const Vector4 &other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }

    INLINE Vector4& Vector4::operator*=(float value)
    {
        x *= value;
        y *= value;
        z *= value;
        w *= value;
        return *this;
    }

    INLINE Vector4& Vector4::operator/=(const Vector4 &other)
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
        return *this;
    }

    INLINE Vector4& Vector4::operator/=(float value)
    {
        auto oneOver = 1.0f / value;
        x *= oneOver;
        y *= oneOver;
        z *= oneOver;
        w *= oneOver;
        return *this;
    }

    INLINE bool Vector4::operator==(const Vector4 &other) const
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    INLINE bool Vector4::operator!=(const Vector4 &other) const
    {
        return x != other.x || y != other.y || z != other.z || w != other.w;
    }

    INLINE float Vector4::operator|(const Vector4 &other) const
    {
        return Dot(*this, other);
    }

    INLINE float Vector4::operator|(const Vector3 &other) const
    {
        return Dot(xyz(), other);
    }

    INLINE float Vector4::operator|(const Vector2 &other) const
    {
        return Dot(xy(), other);
    }

    INLINE Vector2 Vector4::xx() const { return Vector2(x,x); }
    INLINE Vector2 Vector4::xz() const { return Vector2(x,z); }
    INLINE Vector2 Vector4::xw() const { return Vector2(x,w); }
    INLINE Vector2 Vector4::yx() const { return Vector2(y,x); }
    INLINE Vector2 Vector4::yy() const { return Vector2(y,y); }
    INLINE Vector2 Vector4::yw() const { return Vector2(y,w); }
    INLINE Vector2 Vector4::zx() const { return Vector2(z,x); }
    INLINE Vector2 Vector4::zy() const { return Vector2(z,y); }
    INLINE Vector2 Vector4::zz() const { return Vector2(z,z); }
    INLINE Vector2 Vector4::wx() const { return Vector2(w,x); }
    INLINE Vector2 Vector4::wy() const { return Vector2(w,y); }
    INLINE Vector2 Vector4::wz() const { return Vector2(w,z); }
    INLINE Vector2 Vector4::ww() const { return Vector2(w,w); }

    INLINE Vector3 Vector4::xxx() const { return Vector3(x,x,x); }
    INLINE Vector3 Vector4::xxy() const { return Vector3(x,x,y); }
    INLINE Vector3 Vector4::xxz() const { return Vector3(x,x,z); }
    INLINE Vector3 Vector4::xxw() const { return Vector3(x,x,w); }
    INLINE Vector3 Vector4::xyx() const { return Vector3(x,y,x); }
    INLINE Vector3 Vector4::xyy() const { return Vector3(x,y,y); }
    INLINE Vector3 Vector4::xyw() const { return Vector3(x,y,w); }
    INLINE Vector3 Vector4::xzx() const { return Vector3(x,z,x); }
    INLINE Vector3 Vector4::xzy() const { return Vector3(x,z,y); }
    INLINE Vector3 Vector4::xzz() const { return Vector3(x,z,z); }
    INLINE Vector3 Vector4::xzw() const { return Vector3(x,z,w); }
    INLINE Vector3 Vector4::xwx() const { return Vector3(x,w,x); }
    INLINE Vector3 Vector4::xwy() const { return Vector3(x,w,y); }
    INLINE Vector3 Vector4::xwz() const { return Vector3(x,w,z); }
    INLINE Vector3 Vector4::xww() const { return Vector3(x,w,w); }
    INLINE Vector3 Vector4::yxx() const { return Vector3(y,x,x); }
    INLINE Vector3 Vector4::yxy() const { return Vector3(y,x,y); }
    INLINE Vector3 Vector4::yxz() const { return Vector3(y,x,z); }
    INLINE Vector3 Vector4::yxw() const { return Vector3(y,x,w); }
    INLINE Vector3 Vector4::yyx() const { return Vector3(y,y,x); }
    INLINE Vector3 Vector4::yyy() const { return Vector3(y,y,y); }
    INLINE Vector3 Vector4::yyz() const { return Vector3(y,y,z); }
    INLINE Vector3 Vector4::yyw() const { return Vector3(y,y,w); }
    INLINE Vector3 Vector4::yzx() const { return Vector3(y,z,x); }
    INLINE Vector3 Vector4::yzy() const { return Vector3(y,z,y); }
    INLINE Vector3 Vector4::yzz() const { return Vector3(y,z,z); }
    INLINE Vector3 Vector4::ywx() const { return Vector3(y,w,x); }
    INLINE Vector3 Vector4::ywy() const { return Vector3(y,w,y); }
    INLINE Vector3 Vector4::ywz() const { return Vector3(y,w,z); }
    INLINE Vector3 Vector4::yww() const { return Vector3(y,w,w); }
    INLINE Vector3 Vector4::zxx() const { return Vector3(z,x,x); }
    INLINE Vector3 Vector4::zxy() const { return Vector3(z,x,y); }
    INLINE Vector3 Vector4::zxz() const { return Vector3(z,x,z); }
    INLINE Vector3 Vector4::zxw() const { return Vector3(z,x,w); }
    INLINE Vector3 Vector4::zyx() const { return Vector3(z,y,x); }
    INLINE Vector3 Vector4::zyy() const { return Vector3(z,y,y); }
    INLINE Vector3 Vector4::zyz() const { return Vector3(z,y,z); }
    INLINE Vector3 Vector4::zyw() const { return Vector3(z,y,w); }
    INLINE Vector3 Vector4::zzx() const { return Vector3(z,z,x); }
    INLINE Vector3 Vector4::zzy() const { return Vector3(z,z,y); }
    INLINE Vector3 Vector4::zzz() const { return Vector3(z,z,z); }
    INLINE Vector3 Vector4::zzw() const { return Vector3(z,z,w); }
    INLINE Vector3 Vector4::zwx() const { return Vector3(z,w,x); }
    INLINE Vector3 Vector4::zwy() const { return Vector3(z,w,y); }
    INLINE Vector3 Vector4::zwz() const { return Vector3(z,w,z); }
    INLINE Vector3 Vector4::zww() const { return Vector3(z,w,w); }
    INLINE Vector3 Vector4::wxx() const { return Vector3(w,x,x); }
    INLINE Vector3 Vector4::wxy() const { return Vector3(w,x,y); }
    INLINE Vector3 Vector4::wxz() const { return Vector3(w,x,z); }
    INLINE Vector3 Vector4::wxw() const { return Vector3(w,x,w); }
    INLINE Vector3 Vector4::wyx() const { return Vector3(w,y,x); }
    INLINE Vector3 Vector4::wyy() const { return Vector3(w,y,y); }
    INLINE Vector3 Vector4::wyz() const { return Vector3(w,y,z); }
    INLINE Vector3 Vector4::wyw() const { return Vector3(w,y,w); }
    INLINE Vector3 Vector4::wzx() const { return Vector3(w,z,x); }
    INLINE Vector3 Vector4::wzy() const { return Vector3(w,z,y); }
    INLINE Vector3 Vector4::wzz() const { return Vector3(w,z,z); }
    INLINE Vector3 Vector4::wzw() const { return Vector3(w,z,w); }
    INLINE Vector3 Vector4::wwx() const { return Vector3(w,w,x); }
    INLINE Vector3 Vector4::wwy() const { return Vector3(w,w,y); }
    INLINE Vector3 Vector4::wwz() const { return Vector3(w,w,z); }
    INLINE Vector3 Vector4::www() const { return Vector3(w,w,w); }

    INLINE Vector4 Vector4::xxxx() const { return Vector4(x,x,x,x); }
    INLINE Vector4 Vector4::xxxy() const { return Vector4(x,x,x,y); }
    INLINE Vector4 Vector4::xxxz() const { return Vector4(x,x,x,z); }
    INLINE Vector4 Vector4::xxxw() const { return Vector4(x,x,x,w); }
    INLINE Vector4 Vector4::xxyx() const { return Vector4(x,x,y,x); }
    INLINE Vector4 Vector4::xxyy() const { return Vector4(x,x,y,y); }
    INLINE Vector4 Vector4::xxyz() const { return Vector4(x,x,y,z); }
    INLINE Vector4 Vector4::xxyw() const { return Vector4(x,x,y,w); }
    INLINE Vector4 Vector4::xxzx() const { return Vector4(x,x,z,x); }
    INLINE Vector4 Vector4::xxzy() const { return Vector4(x,x,z,y); }
    INLINE Vector4 Vector4::xxzz() const { return Vector4(x,x,z,z); }
    INLINE Vector4 Vector4::xxzw() const { return Vector4(x,x,z,w); }
    INLINE Vector4 Vector4::xxwx() const { return Vector4(x,x,w,x); }
    INLINE Vector4 Vector4::xxwy() const { return Vector4(x,x,w,y); }
    INLINE Vector4 Vector4::xxwz() const { return Vector4(x,x,w,z); }
    INLINE Vector4 Vector4::xxww() const { return Vector4(x,x,w,w); }
    INLINE Vector4 Vector4::xyxx() const { return Vector4(x,y,x,x); }
    INLINE Vector4 Vector4::xyxy() const { return Vector4(x,y,x,y); }
    INLINE Vector4 Vector4::xyxz() const { return Vector4(x,y,x,z); }
    INLINE Vector4 Vector4::xyxw() const { return Vector4(x,y,x,w); }
    INLINE Vector4 Vector4::xyyx() const { return Vector4(x,y,y,x); }
    INLINE Vector4 Vector4::xyyy() const { return Vector4(x,y,y,y); }
    INLINE Vector4 Vector4::xyyz() const { return Vector4(x,y,y,z); }
    INLINE Vector4 Vector4::xyyw() const { return Vector4(x,y,y,w); }
    INLINE Vector4 Vector4::xyzx() const { return Vector4(x,y,z,x); }
    INLINE Vector4 Vector4::xyzy() const { return Vector4(x,y,z,y); }
    INLINE Vector4 Vector4::xyzz() const { return Vector4(x,y,z,z); }
    INLINE Vector4 Vector4::xywx() const { return Vector4(x,y,w,x); }
    INLINE Vector4 Vector4::xywy() const { return Vector4(x,y,w,y); }
    INLINE Vector4 Vector4::xywz() const { return Vector4(x,y,w,z); }
    INLINE Vector4 Vector4::xyww() const { return Vector4(x,y,w,w); }
    INLINE Vector4 Vector4::xzxx() const { return Vector4(x,z,x,x); }
    INLINE Vector4 Vector4::xzxy() const { return Vector4(x,z,x,y); }
    INLINE Vector4 Vector4::xzxz() const { return Vector4(x,z,x,z); }
    INLINE Vector4 Vector4::xzxw() const { return Vector4(x,z,x,w); }
    INLINE Vector4 Vector4::xzyx() const { return Vector4(x,z,y,x); }
    INLINE Vector4 Vector4::xzyy() const { return Vector4(x,z,y,y); }
    INLINE Vector4 Vector4::xzyz() const { return Vector4(x,z,y,z); }
    INLINE Vector4 Vector4::xzyw() const { return Vector4(x,z,y,w); }
    INLINE Vector4 Vector4::xzzx() const { return Vector4(x,z,z,x); }
    INLINE Vector4 Vector4::xzzy() const { return Vector4(x,z,z,y); }
    INLINE Vector4 Vector4::xzzz() const { return Vector4(x,z,z,z); }
    INLINE Vector4 Vector4::xzzw() const { return Vector4(x,z,z,w); }
    INLINE Vector4 Vector4::xzwx() const { return Vector4(x,z,w,x); }
    INLINE Vector4 Vector4::xzwy() const { return Vector4(x,z,w,y); }
    INLINE Vector4 Vector4::xzwz() const { return Vector4(x,z,w,z); }
    INLINE Vector4 Vector4::xzww() const { return Vector4(x,z,w,w); }
    INLINE Vector4 Vector4::xwxx() const { return Vector4(x,w,x,x); }
    INLINE Vector4 Vector4::xwxy() const { return Vector4(x,w,x,y); }
    INLINE Vector4 Vector4::xwxz() const { return Vector4(x,w,x,z); }
    INLINE Vector4 Vector4::xwxw() const { return Vector4(x,w,x,w); }
    INLINE Vector4 Vector4::xwyx() const { return Vector4(x,w,y,x); }
    INLINE Vector4 Vector4::xwyy() const { return Vector4(x,w,y,y); }
    INLINE Vector4 Vector4::xwyz() const { return Vector4(x,w,y,z); }
    INLINE Vector4 Vector4::xwyw() const { return Vector4(x,w,y,w); }
    INLINE Vector4 Vector4::xwzx() const { return Vector4(x,w,z,x); }
    INLINE Vector4 Vector4::xwzy() const { return Vector4(x,w,z,y); }
    INLINE Vector4 Vector4::xwzz() const { return Vector4(x,w,z,z); }
    INLINE Vector4 Vector4::xwzw() const { return Vector4(x,w,z,w); }
    INLINE Vector4 Vector4::xwwx() const { return Vector4(x,w,w,x); }
    INLINE Vector4 Vector4::xwwy() const { return Vector4(x,w,w,y); }
    INLINE Vector4 Vector4::xwwz() const { return Vector4(x,w,w,z); }
    INLINE Vector4 Vector4::xwww() const { return Vector4(x,w,w,w); }
    INLINE Vector4 Vector4::yxxx() const { return Vector4(y,x,x,x); }
    INLINE Vector4 Vector4::yxxy() const { return Vector4(y,x,x,y); }
    INLINE Vector4 Vector4::yxxz() const { return Vector4(y,x,x,z); }
    INLINE Vector4 Vector4::yxxw() const { return Vector4(y,x,x,w); }
    INLINE Vector4 Vector4::yxyx() const { return Vector4(y,x,y,x); }
    INLINE Vector4 Vector4::yxyy() const { return Vector4(y,x,y,y); }
    INLINE Vector4 Vector4::yxyz() const { return Vector4(y,x,y,z); }
    INLINE Vector4 Vector4::yxyw() const { return Vector4(y,x,y,w); }
    INLINE Vector4 Vector4::yxzx() const { return Vector4(y,x,z,x); }
    INLINE Vector4 Vector4::yxzy() const { return Vector4(y,x,z,y); }
    INLINE Vector4 Vector4::yxzz() const { return Vector4(y,x,z,z); }
    INLINE Vector4 Vector4::yxzw() const { return Vector4(y,x,z,w); }
    INLINE Vector4 Vector4::yxwx() const { return Vector4(y,x,w,x); }
    INLINE Vector4 Vector4::yxwy() const { return Vector4(y,x,w,y); }
    INLINE Vector4 Vector4::yxwz() const { return Vector4(y,x,w,z); }
    INLINE Vector4 Vector4::yxww() const { return Vector4(y,x,w,w); }
    INLINE Vector4 Vector4::yyxx() const { return Vector4(y,y,x,x); }
    INLINE Vector4 Vector4::yyxy() const { return Vector4(y,y,x,y); }
    INLINE Vector4 Vector4::yyxz() const { return Vector4(y,y,x,z); }
    INLINE Vector4 Vector4::yyxw() const { return Vector4(y,y,x,w); }
    INLINE Vector4 Vector4::yyyx() const { return Vector4(y,y,y,x); }
    INLINE Vector4 Vector4::yyyy() const { return Vector4(y,y,y,y); }
    INLINE Vector4 Vector4::yyyz() const { return Vector4(y,y,y,z); }
    INLINE Vector4 Vector4::yyyw() const { return Vector4(y,y,y,w); }
    INLINE Vector4 Vector4::yyzx() const { return Vector4(y,y,z,x); }
    INLINE Vector4 Vector4::yyzy() const { return Vector4(y,y,z,y); }
    INLINE Vector4 Vector4::yyzz() const { return Vector4(y,y,z,z); }
    INLINE Vector4 Vector4::yyzw() const { return Vector4(y,y,z,w); }
    INLINE Vector4 Vector4::yywx() const { return Vector4(y,y,w,x); }
    INLINE Vector4 Vector4::yywy() const { return Vector4(y,y,w,y); }
    INLINE Vector4 Vector4::yywz() const { return Vector4(y,y,w,z); }
    INLINE Vector4 Vector4::yyww() const { return Vector4(y,y,w,w); }
    INLINE Vector4 Vector4::yzxx() const { return Vector4(y,z,x,x); }
    INLINE Vector4 Vector4::yzxy() const { return Vector4(y,z,x,y); }
    INLINE Vector4 Vector4::yzxz() const { return Vector4(y,z,x,z); }
    INLINE Vector4 Vector4::yzxw() const { return Vector4(y,z,x,w); }
    INLINE Vector4 Vector4::yzyx() const { return Vector4(y,z,y,x); }
    INLINE Vector4 Vector4::yzyy() const { return Vector4(y,z,y,y); }
    INLINE Vector4 Vector4::yzyz() const { return Vector4(y,z,y,z); }
    INLINE Vector4 Vector4::yzyw() const { return Vector4(y,z,y,w); }
    INLINE Vector4 Vector4::yzzx() const { return Vector4(y,z,z,x); }
    INLINE Vector4 Vector4::yzzy() const { return Vector4(y,z,z,y); }
    INLINE Vector4 Vector4::yzzz() const { return Vector4(y,z,z,z); }
    INLINE Vector4 Vector4::yzzw() const { return Vector4(y,z,z,w); }
    INLINE Vector4 Vector4::yzwx() const { return Vector4(y,z,w,x); }
    INLINE Vector4 Vector4::yzwy() const { return Vector4(y,z,w,y); }
    INLINE Vector4 Vector4::yzwz() const { return Vector4(y,z,w,z); }
    INLINE Vector4 Vector4::yzww() const { return Vector4(y,z,w,w); }
    INLINE Vector4 Vector4::ywxx() const { return Vector4(y,w,x,x); }
    INLINE Vector4 Vector4::ywxy() const { return Vector4(y,w,x,y); }
    INLINE Vector4 Vector4::ywxz() const { return Vector4(y,w,x,z); }
    INLINE Vector4 Vector4::ywxw() const { return Vector4(y,w,x,w); }
    INLINE Vector4 Vector4::ywyx() const { return Vector4(y,w,y,x); }
    INLINE Vector4 Vector4::ywyy() const { return Vector4(y,w,y,y); }
    INLINE Vector4 Vector4::ywyz() const { return Vector4(y,w,y,z); }
    INLINE Vector4 Vector4::ywyw() const { return Vector4(y,w,y,w); }
    INLINE Vector4 Vector4::ywzx() const { return Vector4(y,w,z,x); }
    INLINE Vector4 Vector4::ywzy() const { return Vector4(y,w,z,y); }
    INLINE Vector4 Vector4::ywzz() const { return Vector4(y,w,z,z); }
    INLINE Vector4 Vector4::ywzw() const { return Vector4(y,w,z,w); }
    INLINE Vector4 Vector4::ywwx() const { return Vector4(y,w,w,x); }
    INLINE Vector4 Vector4::ywwy() const { return Vector4(y,w,w,y); }
    INLINE Vector4 Vector4::ywwz() const { return Vector4(y,w,w,z); }
    INLINE Vector4 Vector4::ywww() const { return Vector4(y,w,w,w); }
    INLINE Vector4 Vector4::zxxx() const { return Vector4(z,x,x,x); }
    INLINE Vector4 Vector4::zxxy() const { return Vector4(z,x,x,y); }
    INLINE Vector4 Vector4::zxxz() const { return Vector4(z,x,x,z); }
    INLINE Vector4 Vector4::zxxw() const { return Vector4(z,x,x,w); }
    INLINE Vector4 Vector4::zxyx() const { return Vector4(z,x,y,x); }
    INLINE Vector4 Vector4::zxyy() const { return Vector4(z,x,y,y); }
    INLINE Vector4 Vector4::zxyz() const { return Vector4(z,x,y,z); }
    INLINE Vector4 Vector4::zxyw() const { return Vector4(z,x,y,w); }
    INLINE Vector4 Vector4::zxzx() const { return Vector4(z,x,z,x); }
    INLINE Vector4 Vector4::zxzy() const { return Vector4(z,x,z,y); }
    INLINE Vector4 Vector4::zxzz() const { return Vector4(z,x,z,z); }
    INLINE Vector4 Vector4::zxzw() const { return Vector4(z,x,z,w); }
    INLINE Vector4 Vector4::zxwx() const { return Vector4(z,x,w,x); }
    INLINE Vector4 Vector4::zxwy() const { return Vector4(z,x,w,y); }
    INLINE Vector4 Vector4::zxwz() const { return Vector4(z,x,w,z); }
    INLINE Vector4 Vector4::zxww() const { return Vector4(z,x,w,w); }
    INLINE Vector4 Vector4::zyxx() const { return Vector4(z,y,x,x); }
    INLINE Vector4 Vector4::zyxy() const { return Vector4(z,y,x,y); }
    INLINE Vector4 Vector4::zyxz() const { return Vector4(z,y,x,z); }
    INLINE Vector4 Vector4::zyxw() const { return Vector4(z,y,x,w); }
    INLINE Vector4 Vector4::zyyx() const { return Vector4(z,y,y,x); }
    INLINE Vector4 Vector4::zyyy() const { return Vector4(z,y,y,y); }
    INLINE Vector4 Vector4::zyyz() const { return Vector4(z,y,y,z); }
    INLINE Vector4 Vector4::zyyw() const { return Vector4(z,y,y,w); }
    INLINE Vector4 Vector4::zyzx() const { return Vector4(z,y,z,x); }
    INLINE Vector4 Vector4::zyzy() const { return Vector4(z,y,z,y); }
    INLINE Vector4 Vector4::zyzz() const { return Vector4(z,y,z,z); }
    INLINE Vector4 Vector4::zyzw() const { return Vector4(z,y,z,w); }
    INLINE Vector4 Vector4::zywx() const { return Vector4(z,y,w,x); }
    INLINE Vector4 Vector4::zywy() const { return Vector4(z,y,w,y); }
    INLINE Vector4 Vector4::zywz() const { return Vector4(z,y,w,z); }
    INLINE Vector4 Vector4::zyww() const { return Vector4(z,y,w,w); }
    INLINE Vector4 Vector4::zzxx() const { return Vector4(z,z,x,x); }
    INLINE Vector4 Vector4::zzxy() const { return Vector4(z,z,x,y); }
    INLINE Vector4 Vector4::zzxz() const { return Vector4(z,z,x,z); }
    INLINE Vector4 Vector4::zzxw() const { return Vector4(z,z,x,w); }
    INLINE Vector4 Vector4::zzyx() const { return Vector4(z,z,y,x); }
    INLINE Vector4 Vector4::zzyy() const { return Vector4(z,z,y,y); }
    INLINE Vector4 Vector4::zzyz() const { return Vector4(z,z,y,z); }
    INLINE Vector4 Vector4::zzyw() const { return Vector4(z,z,y,w); }
    INLINE Vector4 Vector4::zzzx() const { return Vector4(z,z,z,x); }
    INLINE Vector4 Vector4::zzzy() const { return Vector4(z,z,z,y); }
    INLINE Vector4 Vector4::zzzz() const { return Vector4(z,z,z,z); }
    INLINE Vector4 Vector4::zzzw() const { return Vector4(z,z,z,w); }
    INLINE Vector4 Vector4::zzwx() const { return Vector4(z,z,w,x); }
    INLINE Vector4 Vector4::zzwy() const { return Vector4(z,z,w,y); }
    INLINE Vector4 Vector4::zzwz() const { return Vector4(z,z,w,z); }
    INLINE Vector4 Vector4::zzww() const { return Vector4(z,z,w,w); }
    INLINE Vector4 Vector4::zwxx() const { return Vector4(z,w,x,x); }
    INLINE Vector4 Vector4::zwxy() const { return Vector4(z,w,x,y); }
    INLINE Vector4 Vector4::zwxz() const { return Vector4(z,w,x,z); }
    INLINE Vector4 Vector4::zwxw() const { return Vector4(z,w,x,w); }
    INLINE Vector4 Vector4::zwyx() const { return Vector4(z,w,y,x); }
    INLINE Vector4 Vector4::zwyy() const { return Vector4(z,w,y,y); }
    INLINE Vector4 Vector4::zwyz() const { return Vector4(z,w,y,z); }
    INLINE Vector4 Vector4::zwyw() const { return Vector4(z,w,y,w); }
    INLINE Vector4 Vector4::zwzx() const { return Vector4(z,w,z,x); }
    INLINE Vector4 Vector4::zwzy() const { return Vector4(z,w,z,y); }
    INLINE Vector4 Vector4::zwzz() const { return Vector4(z,w,z,z); }
    INLINE Vector4 Vector4::zwzw() const { return Vector4(z,w,z,w); }
    INLINE Vector4 Vector4::zwwx() const { return Vector4(z,w,w,x); }
    INLINE Vector4 Vector4::zwwy() const { return Vector4(z,w,w,y); }
    INLINE Vector4 Vector4::zwwz() const { return Vector4(z,w,w,z); }
    INLINE Vector4 Vector4::zwww() const { return Vector4(z,w,w,w); }
    INLINE Vector4 Vector4::wxxx() const { return Vector4(w,x,x,x); }
    INLINE Vector4 Vector4::wxxy() const { return Vector4(w,x,x,y); }
    INLINE Vector4 Vector4::wxxz() const { return Vector4(w,x,x,z); }
    INLINE Vector4 Vector4::wxxw() const { return Vector4(w,x,x,w); }
    INLINE Vector4 Vector4::wxyx() const { return Vector4(w,x,y,x); }
    INLINE Vector4 Vector4::wxyy() const { return Vector4(w,x,y,y); }
    INLINE Vector4 Vector4::wxyz() const { return Vector4(w,x,y,z); }
    INLINE Vector4 Vector4::wxyw() const { return Vector4(w,x,y,w); }
    INLINE Vector4 Vector4::wxzx() const { return Vector4(w,x,z,x); }
    INLINE Vector4 Vector4::wxzy() const { return Vector4(w,x,z,y); }
    INLINE Vector4 Vector4::wxzz() const { return Vector4(w,x,z,z); }
    INLINE Vector4 Vector4::wxzw() const { return Vector4(w,x,z,w); }
    INLINE Vector4 Vector4::wxwx() const { return Vector4(w,x,w,x); }
    INLINE Vector4 Vector4::wxwy() const { return Vector4(w,x,w,y); }
    INLINE Vector4 Vector4::wxwz() const { return Vector4(w,x,w,z); }
    INLINE Vector4 Vector4::wxww() const { return Vector4(w,x,w,w); }
    INLINE Vector4 Vector4::wyxx() const { return Vector4(w,y,x,x); }
    INLINE Vector4 Vector4::wyxy() const { return Vector4(w,y,x,y); }
    INLINE Vector4 Vector4::wyxz() const { return Vector4(w,y,x,z); }
    INLINE Vector4 Vector4::wyxw() const { return Vector4(w,y,x,w); }
    INLINE Vector4 Vector4::wyyx() const { return Vector4(w,y,y,x); }
    INLINE Vector4 Vector4::wyyy() const { return Vector4(w,y,y,y); }
    INLINE Vector4 Vector4::wyyz() const { return Vector4(w,y,y,z); }
    INLINE Vector4 Vector4::wyyw() const { return Vector4(w,y,y,w); }
    INLINE Vector4 Vector4::wyzx() const { return Vector4(w,y,z,x); }
    INLINE Vector4 Vector4::wyzy() const { return Vector4(w,y,z,y); }
    INLINE Vector4 Vector4::wyzz() const { return Vector4(w,y,z,z); }
    INLINE Vector4 Vector4::wyzw() const { return Vector4(w,y,z,w); }
    INLINE Vector4 Vector4::wywx() const { return Vector4(w,y,w,x); }
    INLINE Vector4 Vector4::wywy() const { return Vector4(w,y,w,y); }
    INLINE Vector4 Vector4::wywz() const { return Vector4(w,y,w,z); }
    INLINE Vector4 Vector4::wyww() const { return Vector4(w,y,w,w); }
    INLINE Vector4 Vector4::wzxx() const { return Vector4(w,z,x,x); }
    INLINE Vector4 Vector4::wzxy() const { return Vector4(w,z,x,y); }
    INLINE Vector4 Vector4::wzxz() const { return Vector4(w,z,x,z); }
    INLINE Vector4 Vector4::wzxw() const { return Vector4(w,z,x,w); }
    INLINE Vector4 Vector4::wzyx() const { return Vector4(w,z,y,x); }
    INLINE Vector4 Vector4::wzyy() const { return Vector4(w,z,y,y); }
    INLINE Vector4 Vector4::wzyz() const { return Vector4(w,z,y,z); }
    INLINE Vector4 Vector4::wzyw() const { return Vector4(w,z,y,w); }
    INLINE Vector4 Vector4::wzzx() const { return Vector4(w,z,z,x); }
    INLINE Vector4 Vector4::wzzy() const { return Vector4(w,z,z,y); }
    INLINE Vector4 Vector4::wzzz() const { return Vector4(w,z,z,z); }
    INLINE Vector4 Vector4::wzzw() const { return Vector4(w,z,z,w); }
    INLINE Vector4 Vector4::wzwx() const { return Vector4(w,z,w,x); }
    INLINE Vector4 Vector4::wzwy() const { return Vector4(w,z,w,y); }
    INLINE Vector4 Vector4::wzwz() const { return Vector4(w,z,w,z); }
    INLINE Vector4 Vector4::wzww() const { return Vector4(w,z,w,w); }
    INLINE Vector4 Vector4::wwxx() const { return Vector4(w,w,x,x); }
    INLINE Vector4 Vector4::wwxy() const { return Vector4(w,w,x,y); }
    INLINE Vector4 Vector4::wwxz() const { return Vector4(w,w,x,z); }
    INLINE Vector4 Vector4::wwxw() const { return Vector4(w,w,x,w); }
    INLINE Vector4 Vector4::wwyx() const { return Vector4(w,w,y,x); }
    INLINE Vector4 Vector4::wwyy() const { return Vector4(w,w,y,y); }
    INLINE Vector4 Vector4::wwyz() const { return Vector4(w,w,y,z); }
    INLINE Vector4 Vector4::wwyw() const { return Vector4(w,w,y,w); }
    INLINE Vector4 Vector4::wwzx() const { return Vector4(w,w,z,x); }
    INLINE Vector4 Vector4::wwzy() const { return Vector4(w,w,z,y); }
    INLINE Vector4 Vector4::wwzz() const { return Vector4(w,w,z,z); }
    INLINE Vector4 Vector4::wwzw() const { return Vector4(w,w,z,w); }
    INLINE Vector4 Vector4::wwwx() const { return Vector4(w,w,w,x); }
    INLINE Vector4 Vector4::wwwy() const { return Vector4(w,w,w,y); }
    INLINE Vector4 Vector4::wwwz() const { return Vector4(w,w,w,z); }
    INLINE Vector4 Vector4::wwww() const { return Vector4(w,w,w,w); }

    INLINE const Vector2& Vector4::xy() const { return *(const Vector2*)&x; }
    INLINE const Vector2& Vector4::yz() const { return *(const Vector2*)&y; }
    INLINE const Vector2& Vector4::zw() const { return *(const Vector2*)&z; }
    INLINE const Vector3& Vector4::xyz() const { return *(const Vector3*)&x; }
    INLINE const Vector3& Vector4::yzw() const { return *(const Vector3*)&y; }
    INLINE const Vector4& Vector4::xyzw() const { return *this; }

    INLINE Vector2& Vector4::xy() { return *(Vector2*)&x; }
    INLINE Vector2& Vector4::yz() { return *(Vector2*)&y; }
    INLINE Vector2& Vector4::zw() { return *(Vector2*)&z; }
    INLINE Vector3& Vector4::xyz() { return *(Vector3*)&x; }
    INLINE Vector3& Vector4::yzw() { return *(Vector3*)&y; }
    INLINE Vector4& Vector4::xyzw() { return *this; }

    INLINE Vector2 Vector4::_xy() const { return xy(); }
    INLINE Vector2 Vector4::_yz() const { return yz(); }
    INLINE Vector2 Vector4::_zw() const { return zw(); }
    INLINE Vector3 Vector4::_xyz() const { return xyz(); }
    INLINE Vector3 Vector4::_yzw() const { return yzw(); }
    INLINE Vector4 Vector4::_xyzw() const { return xyzw(); }

    //---

} // base