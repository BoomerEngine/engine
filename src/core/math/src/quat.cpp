/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\quat #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_STRUCT(Quat);
    RTTI_BIND_NATIVE_COMPARE(Quat);
    RTTI_TYPE_TRAIT().noDestructor().fastCopyCompare(); // we must be constructed to identity

    RTTI_PROPERTY(x);
    RTTI_PROPERTY(y);
    RTTI_PROPERTY(z);
    RTTI_PROPERTY(w);
RTTI_END_TYPE();

//--

Quat::Quat(const Vector3& axis, float angleDeg)
{
    if (angleDeg != 0.0f)
    {
        float halfAng = DEG2RAD * angleDeg * 0.5f;
        float s = std::sin(halfAng) / axis.length();
        float c = std::cos(halfAng);
        x = axis.x * s;
        y = axis.y * s;
        z = axis.z * s;
        w = c;
    }
    else
    {
        x = y = z = 0.0f;
        w = 1.0f;
    }
}

float Quat::calcAngle(const Quat& other) const
{
    double dot = (x * other.x) + (y * other.y) + (z * other.z) + (w * other.w);
    dot = std::clamp<double>(dot, -1.0, 1.0);
    return (float)std::acos(2.0 * dot * dot - 1.0);
}

//--

Matrix Quat::toMatrix() const
{
    float d = *this | *this;
    float s = 2.0f / d;
    float xs = x * s, ys = y * s, zs = z * s;
    float wx = w * xs, wy = w * ys, wz = w * zs;
    float xx = x * xs, xy = x * ys, xz = x * zs;
    float yy = y * ys, yz = y * zs, zz = z * zs;

    // Build matrix
    return Matrix(1.0f - (yy + zz), xy - wz, xz + wy,
        xy + wz, 1.0f - (xx + zz), yz - wx,
        xz - wy, yz + wx, 1.0f - (xx + yy));
}

Matrix33 Quat::toMatrix33() const
{
    float d = *this | *this;
    float s = 2.0f / d;
    float xs = x * s, ys = y * s, zs = z * s;
    float wx = w * xs, wy = w * ys, wz = w * zs;
    float xx = x * xs, xy = x * ys, xz = x * zs;
    float yy = y * ys, yz = y * zs, zz = z * zs;

    // Build matrix
    return Matrix33(1.0f - (yy + zz), xy - wz, xz + wy,
                    xy + wz, 1.0f - (xx + zz), yz - wx,
                    xz - wy, yz + wx, 1.0f - (xx + yy));
}

Angles Quat::toRotator() const
{
    return toMatrix().toRotator();
}

//--

static Quat ZERO_Q(0, 0, 0, 0);
static Quat IDENTITY_Q(0, 0, 0, 1);

const Quat& Quat::ZERO()
{
    return ZERO_Q;
}

const Quat& Quat::IDENTITY()
{
    return IDENTITY_Q;
}

//--

END_BOOMER_NAMESPACE()
