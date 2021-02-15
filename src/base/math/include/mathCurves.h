/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

namespace base
{

    //--

    //! calculate the quadratic Bezier coefficients
    extern BASE_MATH_API void BezierFractions(float frac, Vector3& outFraction);

    //! calculate the cubic Bezier coefficients
    extern BASE_MATH_API void BezierFractions(float frac, Vector4& outFraction);

    //! calculate the cubic Hermite coefficients
    extern BASE_MATH_API void HermiteFractions(float frac, Vector4& outFraction);

    //--

    //! lerp scalar value with fractions
    extern BASE_MATH_API float Lerp3(float p0, float p1, float p2, const Vector3& fractions);

    //! lerp vector2 value with fractions
    extern BASE_MATH_API Vector2 Lerp3(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector3& fractions);

    //! lerp vector3 value with fractions
    extern BASE_MATH_API Vector3 Lerp3(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& fractions);

    //! lerp vector4 value with fractions
    extern BASE_MATH_API Vector4 Lerp3(const Vector4& p0, const Vector4& p1, const Vector4& p2, const Vector3& fractions);

    //! lerp color value with fractions, color is assumed to be in SRGB space so it's converted to linear space for blending
    extern BASE_MATH_API Color Lerp3(const Color& p0, const Color& p1, const Color& p2, const Vector3& fractions);

    //! lerp (slerp) quaternion value with fractions
    extern BASE_MATH_API Quat Lerp3(const Quat& p0, const Quat& p1, const Quat& p2, const Vector3& fractions);

    //! lerp angles value with fractions
    extern BASE_MATH_API Angles Lerp3(const Angles& p0, const Angles& p1, const Angles& p2, const Vector3& fractions);

    //! lerp transform with fractions
    extern BASE_MATH_API Transform Lerp3(const Transform& p0, const Transform& p1, const Transform& p2, const Vector3& fractions);

    //--

    //! lerp scalar value with fractions
    extern BASE_MATH_API float Lerp4(float p0, float p1, float p2, float p3, const Vector4& fractions);

    //! lerp vector2 value with fractions
    extern BASE_MATH_API Vector2 Lerp4(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector4& fractions);

    //! lerp vector3 value with fractions
    extern BASE_MATH_API Vector3 Lerp4(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector4& fractions);

    //! lerp vector4 value with fractions
    extern BASE_MATH_API Vector4 Lerp4(const Vector4& p0, const Vector4& p1, const Vector4& p2, const Vector4& p3, const Vector4& fractions);

    //! lerp color value with fractions
    extern BASE_MATH_API Color Lerp4(const Color& p0, const Color& p1, const Color& p2, const Color& p4, const Vector4& fractions);

    //! lerp (slerp) quaternion value with fractions
    extern BASE_MATH_API Quat Lerp4(const Quat& p0, const Quat& p1, const Quat& p2, const Quat& p3, const Vector4& fractions);

    //! lerp angles value with fractions
    extern BASE_MATH_API Angles Lerp4(const Angles& p0, const Angles& p1, const Angles& p2, const Angles& p3, const Vector4& fractions);

    //! lerp transform with fractions
    extern BASE_MATH_API Transform Lerp4(const Transform& p0, const Transform& p1, const Transform& p2, const Transform& p3, const Vector4& fractions);

    //--

    //! interpolate quadratic Bezier
    template< typename T >
    INLINE T Bezier(const T& p0, const T& p1, const T& p2, float frac)
    {
        Vector3 fractions;
        BezierFractions(frac, fractions);
        return Lerp3(p0, p1, p2, fractions);
    }

    //! interpolate cubic Bezier
    template< typename T >
    INLINE T Bezier(const T& p0, const T& p1, const T& p2, const T& p3, float frac)
    {
        Vector4 fractions;
        BezierFractions(frac, fractions);
        return Lerp3(p0, p1, p2, p3, fractions);
    }

    //! interpolate with Hermite
    template< typename T >
    INLINE T Hermite(const T& p0, const T& t0, const T& p1, const T& t1, float frac)
    {
        Vector4 fractions;
        HermiteFractions(frac, fractions);
        return Lerp4(p0, p1, p2, p3, fractions);
    }

    //--

} // base