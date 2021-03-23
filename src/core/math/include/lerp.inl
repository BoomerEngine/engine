/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\convex #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

INLINE LinearInterpolation::LinearInterpolation(float frac_)
    : frac(frac_)
{}

INLINE float LinearInterpolation::lerp(float a, float b) const
{
    return a + (b - a) * frac;
}

INLINE double LinearInterpolation::lerp(double a, double b) const
{
    return a + (b - a) * frac;
}

INLINE Vector2 LinearInterpolation::lerp(const Vector2& a, const Vector2& b) const
{
    return a + (b - a) * frac;
}

INLINE Vector3 LinearInterpolation::lerp(const Vector3& a, const Vector3& b) const
{
    return a + (b - a) * frac;
}

INLINE Vector4 LinearInterpolation::lerp(const Vector4& a, const Vector4& b) const
{
    return a + (b - a) * frac;
}

INLINE ExactPosition LinearInterpolation::lerp(const ExactPosition& a, const ExactPosition& b) const
{
    return a + (b - a) * frac;
}

INLINE Quat LinearInterpolation::lerp(const Quat& a, const Quat& b) const
{
    return Quat(
        lerp(a.x, b.x),
        lerp(a.y, b.y),
        lerp(a.z, b.z),
        lerp(a.w, b.w));
}

INLINE Angles LinearInterpolation::lerpDirect(const Angles& a, const Angles& b) const
{
    return Angles(
        lerp(a.pitch, b.pitch),
        lerp(a.yaw, b.yaw),
        lerp(a.roll, b.roll));
}

//---

INLINE Bezier2Interpolation::Bezier2Interpolation(float frac_)
{
    auto ifrac = 1.0 - frac_;

    f0 = ifrac * ifrac;
    f1 = 2.0f * ifrac * frac_;
    f2 = frac_ * frac_;
}

//---

INLINE Bezier3Interpolation::Bezier3Interpolation(float frac_)
{
    auto ifrac = 1.0 - frac_;
    auto ifrac2 = ifrac * ifrac;
    auto frac2 = frac_ * frac_;

    f0 = ifrac * ifrac2;
    f1 = 3.0f * ifrac2 * frac_;
    f2 = 3.0f * ifrac * frac2;
    f3 = frac_ * frac2;
}

//---

INLINE Hermit3Interpolation::Hermit3Interpolation(float frac_)
{
    auto frac2 = frac_ * frac_;
    auto frac3 = frac2 * frac_;

    f0 = 2.0f * frac3 - 3.0f * frac2 + 1.0f;
    f1 = frac3 - 2.0f * frac2 + frac_;
    f2 = frac3 - frac2;
    f3 = -2.0f * frac3 + 3.0f * frac2;
}

//---

END_BOOMER_NAMESPACE()
