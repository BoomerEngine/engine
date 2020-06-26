/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

//---
// Global math constants

#undef PI
#define PI (3.14159265358979323846)

#undef HALFPI
#define HALFPI (1.57079632679489661923)

#undef TWOPI
#define TWOPI (6.28318530717958647692)

#undef DEG2RAD
#define DEG2RAD (0.01745329251f)

#undef RAD2DEG
#define RAD2DEG (57.2957795131f)

#undef SMALL_EPSILON
#define SMALL_EPSILON (1e-6)

#undef NORMALIZATION_EPSILON
#define NORMALIZATION_EPSILON (1e-8)

#undef PLANE_EPSILON
#define PLANE_EPSILON (1e-6)

#undef VERY_LARGE_FLOAT
#define VERY_LARGE_FLOAT (std::numeric_limits<float>::max())

//---
// Simple math functions that are not in std

namespace base
{
    //--

    /// Plane sides - a special bit mask
    enum PlaneSide
    {
        SIDE_OnPlane = 0,
        SIDE_Positive = 1,
        SIDE_Negative = 2,
        SIDE_Split = 3,
    };

    //--

    class Vector2;
    class Vector3;
    class Vector4;
    class Matrix;
    class Matrix33;
    class Box;
    class Plane;
    class Quat;
    class Transform;
    class Angles;
    class XForm2D;
    class Color;
    class HDRColor;
    class AbsolutePosition;
    class AbsoluteTransform;

    //--

    // normalize angel to -360 to 360 range
    extern BASE_MATH_API float AngleNormalize(float angle);

    // get distance between two angles, it's always in range from -180 to 180 deg
    extern BASE_MATH_API float AngleDistance(float srcAngle, float srcTarget);

    // move given amount of degrees from starting angle towards the target angle
    extern BASE_MATH_API float AngleReach(float srcCurrent, float srcTarget, float move);

    //--

    //! Choose a plane that is the most perpendicular to given directions
    extern BASE_MATH_API bool GetMostPerpendicularPlane(const Vector3 &forward, const Vector3 &axis, const Vector3 &point, Plane &outPlane);

    //! Calculate intersection of ray with plane
    extern BASE_MATH_API bool CalcPlaneRayIntersection(const Vector3& planeNormal, float planeDistance, const Vector3& rayOrigin, const Vector3& rayDir, float rayLength = VERY_LARGE_FLOAT, float* outDistance = nullptr, Vector3* outPosition = nullptr);

    //! Calculate intersection of ray with plane
    extern BASE_MATH_API bool CalcPlaneRayIntersection(const Vector3& planeNormal, const Vector3& planePoint, const Vector3& rayOrigin, const Vector3& rayDir, float rayLength = VERY_LARGE_FLOAT, float* outDistance = nullptr, Vector3* outPosition = nullptr);

    //! Calculate distance to an edge given by two endpoints, can also return closest point
    extern BASE_MATH_API float CalcDistanceToEdge(const Vector3& point, const Vector3& a, const Vector3 &b, Vector3* outClosestPoint);

    //! Calculate two perpendicular vectors (UV)
    extern BASE_MATH_API void CalcPerpendicularVectors(const Vector3& dir, Vector3& outU, Vector3& outV);

    //! Calculate normal of triangle from given vertices, if triangle is degenerated it returns zero vector
    extern BASE_MATH_API Vector3 TriangleNormal(const Vector3 &a, const Vector3 &b, const Vector3 &c);

    //! Calculate normal if the given points create a triangle
    extern BASE_MATH_API bool SafeTriangleNormal(const Vector3 &a, const Vector3 &b, const Vector3 &c, Vector3& outN);

    //! Get normal component of vector
    extern BASE_MATH_API Vector2 NormalPart(const Vector2 &a, const Vector2 &normal);

    //! Get tangent component of vector
    extern BASE_MATH_API Vector2 TangentPart(const Vector2 &a, const Vector2 &normal);

    //! Get normal component of vector
    extern BASE_MATH_API Vector3 NormalPart(const Vector3 &a, const Vector3 &normal);

    //! Get tangent component of vector
    extern BASE_MATH_API Vector3 TangentPart(const Vector3 &a, const Vector3 &normal);

    //! Get normal component of vector
    extern BASE_MATH_API Vector4 NormalPart(const Vector4 &a, const Vector4 &normal);

    //! Get tangent component of vector
    extern BASE_MATH_API Vector4 TangentPart(const Vector4 &a, const Vector4 &normal);

    //! Limit length of the vector
    extern BASE_MATH_API Vector2 ClampLength(const Vector2& a, float maxLength);

    //! Limit length of the vector
    extern BASE_MATH_API Vector3 ClampLength(const Vector3& a, float maxLength);

    //! Limit length of the vector
    extern BASE_MATH_API Vector4 ClampLength(const Vector4& a, float maxLength);

    //! Set length of the vector
    extern BASE_MATH_API Vector2 SetLength(const Vector2& a, float maxLength);

    //! Set length of the vector
    extern BASE_MATH_API Vector3 SetLength(const Vector3& a, float maxLength);

    //! Set length of the vector
    extern BASE_MATH_API Vector4 SetLength(const Vector4& a, float maxLength);

    //! Calculate distance (wrapper) between two angles
    extern BASE_MATH_API float AngleDistance(float srcAngle, float srcTarget);

    //! Move from current to target angle using given movement step
    extern BASE_MATH_API float AngleReach(float srcCurrent, float srcTarget, float move);

    //--

    /// step of the stateless generator
    static INLINE uint64_t StatelessNextUint64(uint64_t& state)
    {
#ifdef PLATFORM_MSVC
        state += UINT64_C(0x60bee2bee120fc15);
        uint64_t tmp;
        tmp = (uint64_t)state * UINT64_C(0xa3b195354a39b70d);
        uint64_t m1 = tmp;
        tmp = (uint64_t)m1 * UINT64_C(0x1b03738712fad5c9);
        uint64_t m2 = tmp;
#else
        state += UINT64_C(0x60bee2bee120fc15);
        __uint128_t tmp;
        tmp = (__uint128_t)state * UINT64_C(0xa3b195354a39b70d);
        uint64_t m1 = (tmp >> 64) ^ tmp;
        tmp = (__uint128_t)m1 * UINT64_C(0x1b03738712fad5c9);
        uint64_t m2 = (tmp >> 64) ^ tmp;
#endif
        return m2;
    }

    //--

#ifdef BUILD_AS_LIBS
    extern BASE_MATH_API TYPE_TLS uint64_t GRandomState;

    INLINE static float Rand()
    {
        auto val  = (uint32_t)StatelessNextUint64(GRandomState) >> 10; // 22 bits
        return (float)val / (float)0x3FFFFF;
    }

    INLINE static uint8_t RandByte()
    {
        return (uint8_t)StatelessNextUint64(GRandomState);
    }

    INLINE static uint32_t RandUint32()
    {
        return (uint32_t)StatelessNextUint64(GRandomState);
    }

    INLINE static uint64_t RandUint64()
    {
        return StatelessNextUint64(GRandomState);
    }
#else
    extern BASE_MATH_API float Rand();

    extern BASE_MATH_API uint8_t RandByte();

    extern BASE_MATH_API uint32_t RandUint32();

    extern BASE_MATH_API uint64_t RandUint64();
#endif

    INLINE static uint32_t RandRange(uint32_t max)
    {
        return (uint32_t)(Rand() * max);
    }

    INLINE static int RandRange(int min, int max)
    {
        return min + (int)std::floor(Rand() * max);
    }

    INLINE static float RandRange(float a, float b)
    {
        return a + (b-a) * Rand();
    }

    INLINE float Frac(float x)
    {
        return x - std::trunc(x);
    }

    INLINE bool IsNearZero(float x, float limit = SMALL_EPSILON)
    {
        return (x > -limit) && (x <= limit);
    }

    INLINE float Snap(float val, float grid)
    {
        if (grid <= 0.0f)
            return val;

        int64_t numGridUnits = (int64_t)std::round(val / grid);
        return numGridUnits * grid;
    }

    INLINE uint8_t FloatTo255(float col)
    {
        if (col <= 0.003921568627450980392156862745098f)
            return 0;
        else if (col >= 0.9960784313725490196078431372549f)
            return 255;
        else
            return (uint8_t)std::lround(col * 255.0f);
    }

    INLINE uint32_t FloatTo256(float col)
    {
        if (col <= 0.00390625f)
            return 0;
        else if (col > 0.99609375f)
            return 256;
        else
            return (uint32_t)std::lround(col * 256.0f);
    }

    INLINE float FloatFrom255(uint8_t col)
    {
        float inv255 = 1.0f / 255.0f;
        return (float)col * inv255;
    }

    INLINE bool IsPow2(uint64_t x)
    {
        return ( x & ( x-1 )) == 0;
    }

    INLINE uint32_t NextPow2(uint32_t v)
    {
        auto x  = v;
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return ++x;
    }

    INLINE uint64_t NextPow2(uint64_t v)
    {
        auto x  = v;
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x |= x >> 32;
        return ++x;
    }

    INLINE uint8_t FloorLog2(uint64_t v)
    {
        auto n  = v;
        auto pos  = 0;
        if ( n >= 1ULL<<32 ) { n >>= 32; pos += 32; }
        if ( n >= 1U<<16 ) { n >>= 16; pos += 16; }
        if ( n >= 1U<< 8 ) { n >>=  8; pos +=  8; }
        if ( n >= 1U<< 4 ) { n >>=  4; pos +=  4; }
        if ( n >= 1U<< 2 ) { n >>=  2; pos +=  2; }
        if ( n >= 1U<< 1 ) {           pos +=  1; }
        return pos;
    }

    INLINE float Bezier2(float p0, float p1, float p2, float frac)
    {
        float ifrac = 1.0f - frac;
        float a = ifrac * ifrac;
        float b = 2.0f * ifrac * frac;
        float c = frac * frac;
        return ( p0 * a ) + ( p1 * b ) + ( p2 * c );
    }

    INLINE float Bezier3(float p0, float p1, float p2, float p3, float frac)
    {
        float ifrac = 1.0f - frac;
        float a = ifrac * ifrac * ifrac;
        float b = 3.0f * ifrac * ifrac * frac;
        float c = 3.0f * ifrac * frac * frac;
        float d = frac * frac * frac;
        return ( p0 * a ) + ( p1 * b ) + ( p2 * c ) + ( p3 * d );
    }

    INLINE float BezierT(float p0, float t0, float p1, float t1, float frac)
    {
        float ifrac = 1.0f - frac;
        float a = ifrac * ifrac * ifrac;
        float b = 3.0f * ifrac * ifrac * frac;
        float c = 3.0f * ifrac * frac * frac;
        float d = frac * frac * frac;
        return ( p0 * a ) + ( (p0+t0) * b ) + ( (p1+t1) * c ) + ( p1 * d );
    }

    INLINE float Hermite(float p0, float t0, float p1, float t1, float frac)
    {
        float frac2 = frac * frac;
        float frac3 = frac2 * frac;
        float a = 2.0f*frac3 - 3.0f*frac2 + 1.0f;
        float b = frac3 - 2.0f*frac2 + frac;
        float c = frac3 - frac2;
        float d = -2.0f*frac3 + 3.0f*frac2;
        return ( p0 * a ) + ( (p0+t0) * b ) + ( (p1+t1) * c ) + ( p1 * d );
    }

    INLINE float AngleNormalize(float angle)
    {
        return angle - (std::trunc(angle / 360.0f) * 360.0f);
    }

    INLINE float Lerp(float valA, float valB, float fraction)
    {
        return valA + fraction * ( valB - valA );
    }

    INLINE double Lerp(double valA, double valB, float fraction)
    {
        return valA + fraction * ( valB - valA );
    }

    INLINE float RadCircle(uint32_t i, uint32_t count)
    {
        auto frac  = (count > 0) ? ((float)i / (float)count) : 0.0f;
        return TWOPI * frac;
    }

} // base