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

/// linear interpolation "engine"
class CORE_MATH_API LinearInterpolation : public NoCopy
{
public:
    INLINE LinearInterpolation(float frac_);

    INLINE float lerp(float a, float b) const;
    INLINE double lerp(double a, double b) const;

    INLINE Vector2 lerp(const Vector2& a, const Vector2& b) const;
    INLINE Vector3 lerp(const Vector3& a, const Vector3& b) const;
    INLINE Vector4 lerp(const Vector4& a, const Vector4& b) const;

    INLINE ExactPosition lerp(const ExactPosition& a, const ExactPosition& b) const;

    INLINE Quat lerp(const Quat& a, const Quat& b) const;
    Quat slerp(const Quat& a, const Quat& b) const;

    INLINE Angles lerpDirect(const Angles& a, const Angles& b) const;
    Angles lerpShortest(const Angles& a, const Angles& b) const;

    Color lerpLinear(const Color& a, const Color& b) const; // assume values are in linear space (ie. no conversion)
    Color lerpGamma(const Color& a, const Color& b) const; // assume values are in gamma space (ie. we convert them to linear space before lerping)

    //--

    EulerTransform lerp(const EulerTransform& a, const EulerTransform& b) const;
    Transform lerp(const Transform& a, const Transform& b) const;

    //--

    float frac;
};

//---

/// quadratic interpolation "engine"
class CORE_MATH_API Bezier2Interpolation : public NoCopy
{
public:
    INLINE Bezier2Interpolation(float frac_);

    //--

    float f0 = 1.0f;
    float f1 = 0.0f;
    float f2 = 0.0f;
};

//---

/// cubic interpolation "engine"
class CORE_MATH_API Bezier3Interpolation : public NoCopy
{
public:
    INLINE Bezier3Interpolation(float frac_);

    //--

    float f0 = 1.0f;
    float f1 = 0.0f;
    float f2 = 0.0f;
    float f3 = 0.0f;
};

//---

/// cubic interpolation "engine"
class CORE_MATH_API Hermit3Interpolation : public NoCopy
{
public:
    INLINE Hermit3Interpolation(float frac_);

    //--

    float f0 = 1.0f;
    float f1 = 0.0f;
    float f2 = 0.0f;
    float f3 = 0.0f;
};

//---

END_BOOMER_NAMESPACE()
