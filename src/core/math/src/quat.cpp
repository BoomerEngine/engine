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

Vector3 Quat::transformVector(const Vector3& v) const
{
    Vector3 ret;
    transformVector(v, ret);
    return ret;
}

float Quat::calcAngle(const Quat& other) const
{
    double dot = (x * other.x) + (y * other.y) + (z * other.z) + (w * other.w);
    dot = std::clamp<double>(dot, -1.0, 1.0);
    return (float)std::acos(2.0 * dot * dot - 1.0);
}

void Quat::transformVector(const Vector3& v, Vector3& ret) const
{
    toMatrix().transformVector(v, ret);
}

Vector3 Quat::transformInvVector(const Vector3& v) const
{
    Vector3 ret;
    transformInvVector(v, ret);
    return ret;
}

void Quat::transformInvVector(const Vector3& v, Vector3& ret) const
{
    inverted().toMatrix().transformVector(v, ret);
}

//--

Matrix Quat::toMatrix() const
{
    float d = Dot(*this, *this);
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
    float d = Dot(*this, *this);
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

Quat LinearLerp(const Quat &a, const Quat &b, float fraction)
{
    float x = Lerp(a.x, b.x, fraction);
    float y = Lerp(a.y, b.y, fraction);
    float z = Lerp(a.z, b.z, fraction);
    float w = Lerp(a.w, b.w, fraction);
    return Quat(x, y, z, w);
}

Quat Lerp(const Quat &a, const Quat &b, float fraction)
{
    float cosOmega = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    float rot[4];

    // Align
    if (cosOmega < 0.0f)
    {
        cosOmega = -cosOmega;
        rot[0] = -b.x;
        rot[1] = -b.y;
        rot[2] = -b.z;
        rot[3] = -b.w;
    }
    else
    {
        rot[0] = b.x;
        rot[1] = b.y;
        rot[2] = b.z;
        rot[3] = b.w;
    }

    float omega, sinOmega, scaleRot0, scaleRot1;
    if ((1.0f - cosOmega) > 0.00001f)
    {
        omega = std::acos(cosOmega);
        sinOmega = std::sin(omega);
        scaleRot0 = std::sin((1.0f - fraction) * omega) / sinOmega;
        scaleRot1 = std::sin(fraction * omega) / sinOmega;
    }
    else
    {
        scaleRot0 = 1.0f - fraction;
        scaleRot1 = fraction;
    }

    return Quat(scaleRot0 * a.x + scaleRot1 * rot[0],
                scaleRot0 * a.y + scaleRot1 * rot[1],
                scaleRot0 * a.z + scaleRot1 * rot[2],
                scaleRot0 * a.w + scaleRot1 * rot[3]);
}

float Dot(const Quat &a, const Quat &b)
{
    return (a.x*b.x) + (a.y*b.y) + (a.z*b.z) + (a.w*b.w);
}

Quat Concat(const Quat &a, const Quat &b)
{
    return Quat(a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
                a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
                a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
                a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z);
}

//--

END_BOOMER_NAMESPACE()
