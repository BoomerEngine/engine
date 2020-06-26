/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\xform2D #]
***/

#pragma once

namespace base
{
    //--

    INLINE XForm2D::XForm2D()
    {
        t[0] = 1.0f; t[1] = 0.0f;
        t[2] = 0.0f; t[3] = 1.0f;
        t[4] = 0.0f; t[5] = 0.0f;
    }

    INLINE XForm2D::XForm2D(const float* data)
    {
        memcpy(&t, data, sizeof(t));
    }

    INLINE XForm2D::XForm2D(float t00, float t01, float t10, float t11, float tx, float ty)
    {
        t[0] = t00;
        t[1] = t01;
        t[2] = t10;
        t[3] = t11;
        t[4] = tx;
        t[5] = ty;
    }

    INLINE XForm2D::XForm2D(float tx, float ty)
    {
        t[0] = 1.0f;
        t[1] = 0.0f;
        t[2] = 0.0f;
        t[3] = 1.0f;
        t[4] = tx;
        t[5] = ty;
    }

    INLINE XForm2D& XForm2D::identity()
    {
        t[0] = 1.0f; t[1] = 0.0f;
        t[2] = 0.0f; t[3] = 1.0f;
        t[4] = 0.0f; t[5] = 0.0f;
        return *this;
    }

    INLINE XForm2D& XForm2D::translation(const Vector2& trans)
    {
        t[4] = trans.x;
        t[5] = trans.y;
        return *this;
    }

    INLINE XForm2D& XForm2D::translation(float x, float y)
    {
        t[4] = x;
        t[5] = y;
        return *this;
    }

    INLINE float XForm2D::transformX(float x, float y) const
    {
        return (x * t[0]) + (y * t[2]) + t[4];
    }

    INLINE float XForm2D::transformY(float x, float y) const
    {
        return (x * t[1]) + (y * t[3]) + t[5];
    }

    INLINE double XForm2D::det() const
    {
        return ((double)t[0]*(double)t[3]) - ((double)t[1]*(double)t[2]);
    }

    ALWAYS_INLINE Vector2 XForm2D::transformPoint(const Vector2& pos) const
    {
        return Vector2(pos.x * t[0] + pos.y * t[2] + t[4], pos.x * t[1] + pos.y * t[3] + t[5]);
    }

    ALWAYS_INLINE Vector2 XForm2D::transformVector(const Vector2& pos) const
    {
        return Vector2(pos.x * t[0] + pos.y * t[2], pos.x * t[1] + pos.y * t[3]);
    }

    //--

    INLINE const Vector2& XForm2D::translation() const
    {
        return *(const Vector2*)(&t[4]);
    }

    INLINE XForm2D XForm2D::operator*(const XForm2D& other) const
    {
        return Concat(*this, other);
    }

    INLINE XForm2D& XForm2D::operator*=(const XForm2D& other)
    {
        *this = Concat(*this, other);
        return *this;
    }

    INLINE XForm2D XForm2D::operator+(const Vector2& delta) const
    {
        return XForm2D(*this) += delta;
    }

    INLINE XForm2D& XForm2D::operator+=(const Vector2& delta)
    {
        t[4] += delta.x;
        t[5] += delta.y;
        return *this;
    }

    INLINE XForm2D XForm2D::operator-(const Vector2& delta) const
    {
        return XForm2D(*this) -= delta;
    }

    INLINE XForm2D& XForm2D::operator-=(const Vector2& delta)
    {
        t[4] -= delta.x;
        t[5] -= delta.y;
        return *this;
    }

    INLINE XForm2D XForm2D::operator~() const
    {
        return inverted();
    }

    INLINE bool XForm2D::operator==(const XForm2D& other) const
    {
        return 0 == memcmp(&t, &other.t, sizeof(t));
    }

    INLINE bool XForm2D::operator!=(const XForm2D& other) const
    {
        return 0 != memcmp(&t, &other.t, sizeof(t));
    }

    //--

} // base