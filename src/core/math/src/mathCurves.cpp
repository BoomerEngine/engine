/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#include "build.h"
#include "mathCurves.h"

BEGIN_BOOMER_NAMESPACE()

//---

void BezierFractions(float frac, Vector3& outFractions)
{
    float ifrac = 1.0f - frac;
    outFractions.x = ifrac * ifrac;
    outFractions.y = 2.0f * ifrac * frac;
    outFractions.z = frac * frac;
}

void BezierFractions(float frac, Vector4& outFractions)
{
    float ifrac = 1.0f - frac;
    outFractions.x = ifrac * ifrac * ifrac;
    outFractions.y = 3.0f * ifrac * ifrac * frac;
    outFractions.z = 3.0f * ifrac * frac * frac;
    outFractions.w = frac * frac * frac;
}

void HermiteFractions(float frac, Vector4& outFractions)
{
    float frac2 = frac * frac;
    float frac3 = frac2 * frac;
    outFractions.x = 2.0f * frac3 - 3.0f * frac2 + 1.0f;
    outFractions.y = frac3 - 2.0f * frac2 + frac;
    outFractions.z = frac3 - frac2;
    outFractions.w = -2.0f * frac3 + 3.0f * frac2;
}

//---

template< typename T >
static INLINE T DefaultLerp3(const T& p0, const T& p1, const T& p2, const Vector3& fractions)
{
    auto ret = p0 * fractions.x;
    ret += (p1 * fractions.y);
    ret += (p2 * fractions.z);
    return ret;
}

float Lerp3(float p0, float p1, float p2, const Vector3& fractions)
{
    return DefaultLerp3(p0, p1, p2, fractions);
}

Vector2 Lerp3(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector3& fractions)
{
    return DefaultLerp3(p0, p1, p2, fractions);
}

Vector3 Lerp3(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& fractions)
{
    return DefaultLerp3(p0, p1, p2, fractions);
}

Vector4 Lerp3(const Vector4& p0, const Vector4& p1, const Vector4& p2, const Vector3& fractions)
{
    return DefaultLerp3(p0, p1, p2, fractions);
}

Color Lerp3(const Color& p0, const Color& p1, const Color& p2, const Vector3& fractions)
{
    return Color::FromVectorSRGBFast(DefaultLerp3(p0.toVectorSRGB(), p1.toVectorSRGB(), p2.toVectorSRGB(), fractions));
}

Quat Lerp3(const Quat& p0, const Quat& p1, const Quat& p2, const Vector3& fractions)
{
    Vector4 ret = DefaultLerp3((const Vector4&)p0, (const Vector4&)p1, (const Vector4&)p2, fractions);
    return (const Quat&)ret;
}

Angles Lerp3(const Angles& p0, const Angles& p1, const Angles& p2, const Vector3& fractions)
{
    return DefaultLerp3(p0, p1, p2, fractions);
}

Transform Lerp3(const Transform& p0, const Transform& p1, const Transform& p2, const Vector3& fractions)
{
    auto t = Lerp3(p0.T, p1.T, p2.T, fractions);
    auto r = Lerp3(p0.R, p1.R, p2.R, fractions);
    auto s = Lerp3(p0.S, p1.S, p2.S, fractions);
    return Transform(t, r, s);
}

//---

template< typename T >
static INLINE T DefaultLerp4(const T& p0, const T& p1, const T& p2, const T& p3, const Vector4& fractions)
{
    auto ret = p0 * fractions.x;
    ret += (p1 * fractions.y);
    ret += (p2 * fractions.z);
    ret += (p3 * fractions.w);
    return ret;
}

float Lerp4(float p0, float p1, float p2, float p3, const Vector4& fractions)
{
    return DefaultLerp4(p0, p1, p2, p3, fractions);
}

Vector2 Lerp4(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector4& fractions)
{
    return DefaultLerp4(p0, p1, p2, p3, fractions);
}

Vector3 Lerp4(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector4& fractions)
{
    return DefaultLerp4(p0, p1, p2, p3, fractions);
}

Vector4 Lerp4(const Vector4& p0, const Vector4& p1, const Vector4& p2, const Vector4& p3, const Vector4& fractions)
{
    return DefaultLerp4(p0, p1, p2, p3, fractions);
}

Color Lerp4(const Color& p0, const Color& p1, const Color& p2, const Color& p3, const Vector4& fractions)
{
    return Color::FromVectorSRGBFast(DefaultLerp4(p0.toVectorSRGB(), p1.toVectorSRGB(), p2.toVectorSRGB(), p3.toVectorSRGB(), fractions));
}

Quat Lerp4(const Quat& p0, const Quat& p1, const Quat& p2, const Quat& p3, const Vector4& fractions)
{
    Vector4 ret = DefaultLerp4((const Vector4&)p0, (const Vector4&)p1, (const Vector4&)p2, (const Vector4&)p3, fractions);
    return (const Quat&)ret;
}

Angles Lerp4(const Angles& p0, const Angles& p1, const Angles& p2, const Angles& p3, const Vector4& fractions)
{
    return DefaultLerp4(p0, p1, p2, p3, fractions);
}

Transform Lerp4(const Transform& p0, const Transform& p1, const Transform& p2, const Transform& p3, const Vector4& fractions)
{
    auto t = Lerp4(p0.T, p1.T, p2.T, p3.T, fractions);
    auto r = Lerp4(p0.R, p1.R, p2.R, p3.R, fractions);
    auto s = Lerp4(p0.S, p1.S, p2.S, p3.S, fractions);
    return Transform(t, r, s);
}

//--

END_BOOMER_NAMESPACE()
