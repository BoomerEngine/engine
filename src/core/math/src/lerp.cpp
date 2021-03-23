/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\box #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//--

Quat LinearInterpolation::slerp(const Quat& a, const Quat& b) const
{
    float cosOmega = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
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
        scaleRot0 = std::sin((1.0f - frac) * omega) / sinOmega;
        scaleRot1 = std::sin(frac * omega) / sinOmega;
    }
    else
    {
        scaleRot0 = 1.0f - frac;
        scaleRot1 = frac;
    }

    return Quat(scaleRot0 * a.x + scaleRot1 * rot[0],
        scaleRot0 * a.y + scaleRot1 * rot[1],
        scaleRot0 * a.z + scaleRot1 * rot[2],
        scaleRot0 * a.w + scaleRot1 * rot[3]);
}

Angles LinearInterpolation::lerpShortest(const Angles& a, const Angles& b) const
{
    return Angles(
        lerp(AngleNormalize(a.pitch), AngleNormalize(b.pitch)),
        lerp(AngleNormalize(a.yaw), AngleNormalize(b.yaw)),
        lerp(AngleNormalize(a.roll), AngleNormalize(b.roll)));
}

Color LinearInterpolation::lerpLinear(const Color& a, const Color& b) const
{
    return Color::FromVectorLinear(lerp(a.toVectorLinear(), b.toVectorLinear()));
}

Color LinearInterpolation::lerpGamma(const Color& a, const Color& b) const
{
    return Color::FromVectorSRGBFast(lerp(a.toVectorSRGB(), b.toVectorSRGB()));
}

EulerTransform LinearInterpolation::lerp(const EulerTransform& a, const EulerTransform& b) const
{
    return EulerTransform(
        lerp(a.T, a.T),
        lerpDirect(a.R, a.R),
        lerp(a.S, a.S));
}

Transform LinearInterpolation::lerp(const Transform& a, const Transform& b) const
{
    return Transform(
        lerp(a.T, a.T),
        slerp(a.R, a.R),
        lerp(a.S, a.S));
}

//--

END_BOOMER_NAMESPACE()
